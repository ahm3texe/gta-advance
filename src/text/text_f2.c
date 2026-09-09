/* Place a glyph in the text layer — 0x08064020-0x0806417F
 *
 * DrawTextAt in draw_text.c calls this for each character. It clips,
 * normalizes characters, constructs destination tile addresses, and calls
 * the pixel blender BlendGlyphAcrossTiles (text_f1.c).
 *
 * Flow:
 * 1. Clip x > 239, y > 159, y < 0, x <= -16.
 * 2. Substitute 146 -> 39 (same as GlyphAdvance in draw_text.c), then _toupper;
 *    the glyph table contains uppercase characters only.
 * 3. Read width EARLY through GetGlyphWidth, needed both for rejection and
 *    deciding whether a second tile column is required. The ROM keeps it in
 *    a stack slot (sub sp,#4), a compiler spill with no source-level equivalent.
 * 4. Skip 95 ('_') and control characters below 32.
 * 5. Increment odd x: destination halfwords hold pixel pairs, requiring alignment.
 * 6. Reject x + width <= 0, where the whole glyph lies left of the screen.
 *
 * Layout: row = y >> 3, column = x >> 3, subpixel = x & 7 (the ROM uses
 * x - (column << 3), not a mask). Destination byte address = gTextVramBase
 * + column*64 + row*gTextRowStride*64.
 *
 * TWO GLYPH LAYOUTS selected by gHalfLineSpacing:
 * Nonzero: 8x8 glyph, 64 bytes per tile, ONE call.
 * Zero: 16x16 glyph, 256 bytes, FOUR tiles ordered +0x00 upper-left, +0x40
 * upper-right, +0x80 lower-left, +0xC0 lower-right. Lower tiles use
 * dest + stride*64. Draw the right column only if width > 8; increment x
 * by 8 and recompute column/subpixel. clear_text_area.c interprets the flag
 * consistently (0 means full line height).
 *
 * gTextRowStride is reread AFTER every call, since calls may modify memory.
 * Its address stays in sl as a natural consequence of one literal-pool load,
 * not a separate source local.
 *
 * 0x080640CA-0x080640DB and 0x08064174-0x0806417F are literal pools of four
 * RAM addresses, NOT CODE, despite disassembler output.
 *
 * TWO MEASURED FIXES (starting from 356/352 bytes):
 * 1. WORD-WIDTH FIRST PARAMETER, not u8 (rule 15). u8 ch inserts lsls #24 /
 *    lsrs #24 at entry; the ROM instead starts with adds r4,r0,#0. u8 gives
 *    311/356 differences, word width 141/352. Normalization also shifts all
 *    register allocation. At the time of this note, clear_text_area.c and
 *    draw_text.c still declared extern void PlaceGlyph(u8 ch, s32 x, s32 y).
 *    The call ABI is the same (a u8 value already arrives in r0), and both
 *    files matched, but the declaration/definition types differed; those
 *    declarations were identified for conversion to s32 on the next edit.
 * 2. ONE VARIABLE for raw character and table index (rule 27). A separate
 *    index produced adds r3,r0,#0 + adds r4,r3,#0 and swapped x/y between
 *    r5/r6. Updating ch in place (_toupper, then ch -= 32) fixed both:
 *    141 -> 0.
 *
 * REJECTED: u8 ch (311/356) and a separate index (141/352). Three versions
 * sufficed, so no volatile, separate base locals or short-lived temporary
 * blocks were needed to adjust allocation.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/text/text_f2.c
 */

#include "gba_types.h"

#define GLYPH_FIRST      32   /* first table character (space) */
#define GLYPH_SUBSTITUTE 146  /* special byte found in text */
#define GLYPH_APOSTROPHE 39   /* replacement character */
#define GLYPH_SKIP       95   /* underscore is never drawn */
#define SCREEN_RIGHT     239
#define SCREEN_BOTTOM    159
#define LEFT_LIMIT       16   /* x <= -16 places the glyph entirely left of the screen */
#define TILE_SHIFT       3    /* 8 pixels = 1 tile */
#define TILE_ORDER       6    /* 8bpp tile = 64 bytes */
#define GLYPH_ORDER      8    /* 16x16 glyph = 256 bytes */
#define GLYPH_HALF       8    /* narrow glyph width */
#define TILE_TOP_RIGHT   0x40
#define TILE_BOT_LEFT    0x80
#define TILE_BOT_RIGHT   0xC0

extern u32  gTextRowStride;
extern u8  *gTextVramBase;
extern u8  *gGlyphTiles;
extern u32  gHalfLineSpacing;

extern u8   _toupper(u8 ch);
extern s32  GetGlyphWidth(u32 ch);
extern void BlendGlyphAcrossTiles(u16 *dest, s32 subX, const u8 *src, s32 col);

/* 0x08064020 */
void PlaceGlyph(s32 ch, s32 x, s32 y)
{
    s32 width;
    s32 row;
    s32 col;
    s32 subX;
    u8 *glyph;
    u8 *dest;

    if (x > SCREEN_RIGHT)
        return;
    if (y > SCREEN_BOTTOM)
        return;
    if (y < 0)
        return;
    if (x <= -LEFT_LIMIT)
        return;

    if (ch == GLYPH_SUBSTITUTE)
        ch = GLYPH_APOSTROPHE;

    ch = _toupper((u8)ch);
    width = GetGlyphWidth(ch);
    if (ch == GLYPH_SKIP)
        return;

    ch -= GLYPH_FIRST;
    if (ch < 0)
        return;

    if (x & 1)
        x++;
    if (x + width <= 0)
        return;

    row = y >> TILE_SHIFT;
    col = x >> TILE_SHIFT;
    if (row < 0)
        return;
    subX = x - (col << TILE_SHIFT);

    if (gHalfLineSpacing != 0) {
        glyph = gGlyphTiles + (ch << TILE_ORDER);
        dest = gTextVramBase + (col << TILE_ORDER)
             + ((row * gTextRowStride) << TILE_ORDER);
        BlendGlyphAcrossTiles((u16 *)dest, subX, glyph, col);
        return;
    }

    glyph = gGlyphTiles + (ch << GLYPH_ORDER);
    dest = gTextVramBase + (col << TILE_ORDER)
         + ((row * gTextRowStride) << TILE_ORDER);
    BlendGlyphAcrossTiles((u16 *)dest, subX, glyph, col);
    BlendGlyphAcrossTiles((u16 *)(dest + (gTextRowStride << TILE_ORDER)), subX,
                 glyph + TILE_BOT_LEFT, col);

    if (width <= GLYPH_HALF)
        return;

    x += GLYPH_HALF;
    col = x >> TILE_SHIFT;
    subX = x - (col << TILE_SHIFT);
    dest = gTextVramBase + (col << TILE_ORDER)
         + ((row * gTextRowStride) << TILE_ORDER);
    BlendGlyphAcrossTiles((u16 *)dest, subX, glyph + TILE_TOP_RIGHT, col);
    BlendGlyphAcrossTiles((u16 *)(dest + (gTextRowStride << TILE_ORDER)), subX,
                 glyph + TILE_BOT_RIGHT, col);
}
