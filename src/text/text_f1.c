/* Blend a glyph across adjacent tiles — 0x08063ED8-0x0806401D
 *
 * Draw an eight-pixel-wide glyph into an 8bpp text layer of 8x8 tiles. Nonzero
 * subX makes it span TWO tiles. Two copies of the inner loop handle remaining
 * columns of the current tile (firstPairs), then leading columns of the next
 * (secondPairs). Their sum is always four pixel PAIRS = eight pixels.
 *
 * PlaceGlyph passes r0 = destination tile, r1 = subX (x & 7, even),
 * r2 = glyph data, r3 = tile column (x >> 3). Columns range from -1 to 29;
 * 30*8 = 240 screen pixels. At col == -1 only the RIGHT part is visible: skip
 * the first loop but advance src by the same amount. Test nextCol for the
 * second tile, since it lies at col+1.
 *
 * Each destination halfword holds two 8bpp pixels, with the left in its low
 * byte. A zero glyph byte is TRANSPARENT and preserves the existing pixel.
 * Otherwise add gFontIndex (the selected font's tile/palette base index) and
 * truncate to 16 bits.
 *
 * MEASURED SOURCE-FORM RULES (each independently closed differences):
 * 1. POINTER WALK: advance destination/source by 8 bytes per row (four
 *    destination halfwords). After eight rows, subtract 62 bytes from both,
 *    giving net +2 bytes, one pixel pair. Separate d/s locals and dest++
 *    reload the base; the ROM walks one pointer and subtracts the difference.
 * 2. SEPARATE nextCol: col++ creates one pseudo spanning the function and
 *    adds mov ip,r3 at entry. The ROM leaves the parameter in r3 and assigns
 *    the separate col+1 pseudo to sl (rule 27).
 * 3. SEPARATE LOOP COUNTERS: sharing n1/n2 extends lifetime across both loops,
 *    lowers priority and spills the counter.
 * 4. DECREMENT IN THE TEST (while (--n1 != 0)), not at the body start. A body
 *    n1-- adds a depth-0 reference: refs 8 -> floor_log2 3 -> priority 0.48,
 *    above row's 0.412. The counter takes a low register and reload spills
 *    it. Decrementing in the test puts it in ip and row in r7: 193 -> 93
 *    differing bytes from this change alone.
 * 5. SEPARATE SECOND COUNTER (n2 = secondPairs): counting with secondPairs
 *    adds depth-1 references, raising it above nextCol. It then takes ip and
 *    nextCol spills, opposite to the ROM. Separate n2 leaves secondPairs with
 *    three references and on the stack, while nextCol takes sl: 93 -> 48.
 * 6. SOURCE ORDER: n1 = firstPairs BEFORE nextCol = col + 1 (rule 19).
 *    Reversed order gives 89 differing bytes; correct order gives 48.
 * 7. IN-PLACE MASK: cur &= 0xFF, without a separate lo. The ROM's ands r3,r0
 *    leaves the result in cur's register; a separate local pushes hi into r3
 *    instead of r6 (rule 33 here).
 * 8. WRITE BACK TO THE SOURCE-BYTE LOCAL (srcLo = ...), not pixLo/pixHi: the
 *    ROM shares the result register with srcLo/srcHi. 42 -> 4 differing bytes.
 * 9. srcLo/srcHi are u16 with IMPLICIT truncation. Explicit
 *    (u16)(srcLo + gFontIndex) reverses addition operands (adds r0,r0,r2
 *    instead of ROM adds r0,r2,r0). This accounts for the last four bytes.
 *    & 0xFFFF disrupts the whole result (243/334).
 *
 * REJECTED: while (n-- != 0) (two pseudos, counter spills); for (row=0; row<8;
 * row++) (signed countdown, bge instead of bne); reversing lo/hi calculation
 * order (single-byte ldrb reads, 322 bytes); defining n1 before if/else
 * (330/208); caching gFontIndex in a local (338/246); guarding with firstPairs
 * (no effect, agbcc merges it).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/text/text_f1.c   -> BYTE-MATCHING 326/326
 */

#include "gba_types.h"

#define LAST_TILE_COL   29   /* 240 pixels = 30 tile columns */
#define GLYPH_ROWS       8   /* tile height */
#define DEST_ROW_STEP    4   /* 8bpp tile row = 8 bytes = 4 halfwords */
#define SRC_ROW_SKIP     6   /* 8-byte row; two bytes already read */
#define DEST_ROW_BACK   31   /* 8*4 - 1: net +1 halfword after eight rows */
#define SRC_ROW_BACK    62   /* 8*8 - 2: net +2 bytes after eight rows */
#define NEXT_TILE       32   /* 64 bytes = one 8bpp tile, in halfwords */
#define TAIL_TO_TILE    28   /* distance to the next tile after the first loop */

extern u32 gFontIndex;

/* 0x08063ED8 */
void BlendGlyphAcrossTiles(u16 *dest, s32 subX, const u8 *src, s32 col)
{
    s32 firstPairs;
    s32 secondPairs;
    s32 rem;
    s32 nextCol;
    s32 n1;
    s32 n2;
    s32 row1;
    s32 row2;
    u16 cur;
    u32 hi;
    u16 srcLo;
    u16 srcHi;

    if (col > LAST_TILE_COL)
        return;
    if (col <= -2)
        return;

    /* Constant 8 is used twice; the ROM keeps it in a single register too. */
    rem = 8 - subX;
    if (rem <= 7) {
        firstPairs = rem >> 1;
        secondPairs = (8 - rem) >> 1;
    } else {
        firstPairs = 4;
        secondPairs = 0;
    }

    if (col < 0) {
        /* Left tile is off-screen: only advance the pointers. */
        dest += NEXT_TILE;
        src += firstPairs * 2;
        nextCol = col + 1;
    } else {
        dest += subX >> 1;
        n1 = firstPairs;
        nextCol = col + 1;
        if (n1 != 0) {
            do {
                for (row1 = GLYPH_ROWS; row1 != 0; row1--) {
                    cur = *dest;
                    hi = (cur & 0xFF00) >> 8;
                    cur &= 0xFF;
                    srcLo = *src++;
                    srcHi = *src++;
                    if (srcLo == 0)
                        srcLo = cur;
                    else
                        srcLo = srcLo + gFontIndex;
                    if (srcHi == 0)
                        srcHi = hi;
                    else
                        srcHi = srcHi + gFontIndex;
                    *dest = (u16)((srcHi << 8) | srcLo);
                    dest += DEST_ROW_STEP;
                    src += SRC_ROW_SKIP;
                }
                dest -= DEST_ROW_BACK;
                src -= SRC_ROW_BACK;
            } while (--n1 != 0);
        }
        dest += TAIL_TO_TILE;
    }

    if (nextCol > LAST_TILE_COL)
        return;
    n2 = secondPairs;
    if (n2 == 0)
        return;

    do {
        for (row2 = GLYPH_ROWS; row2 != 0; row2--) {
            cur = *dest;
            hi = (cur & 0xFF00) >> 8;
            cur &= 0xFF;
            srcLo = *src++;
            srcHi = *src++;
            if (srcLo == 0)
                srcLo = cur;
            else
                srcLo = srcLo + gFontIndex;
            if (srcHi == 0)
                srcHi = hi;
            else
                srcHi = srcHi + gFontIndex;
            *dest = (u16)((srcHi << 8) | srcLo);
            dest += DEST_ROW_STEP;
            src += SRC_ROW_SKIP;
        }
        dest -= DEST_ROW_BACK;
        src -= SRC_ROW_BACK;
    } while (--n2 != 0);
}
