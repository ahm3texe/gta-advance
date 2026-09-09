/* SPLIT OFF from band_a.c: the project's region model wants ONE CONTIGUOUS
 * ROM range per file.  Because these functions are scattered in the ROM they
 * cannot be kept in one file (add_c_region would say "region mismatch").
 * Besides, since agbcc_build.py links the translation unit at min(address),
 * functions containing external calls would end up at the wrong address.
 */

/* Band A -- nine small functions from between 0x080308EC and 0x08031BF4.
 *
 * THE FILE HOLDS SIX FUNCTIONS, NOT NINE.  Three of them want symbols that are
 * not in data/ram_map.csv and a fourth wants the gRam02025810 block; the
 * reason is written in place for each of them below.  Writing a body with a
 * missing symbol would make agbcc_build.py sys.exit and render THE WHOLE FILE
 * unverifiable, so only a note was left.
 *
 * ---------------------------------------------------------------------
 * ONE FILE / SCATTERED ADDRESSES: ABOUT THE `bl` OFFSETS
 * ---------------------------------------------------------------------
 * agbcc_build.py links THE ENTIRE translation unit at the SMALLEST ROM address
 * among the functions defined in it (`base = min(...)`) and lays the functions
 * out back to back in source order.  The functions in this file are NOT
 * contiguous in the ROM (there are other functions between them), so every one
 * after the first lands at the wrong address.  Even when a function body is
 * identical to the ROM's, any `bl` inside it has a shifted relative offset and
 * `make c-match` reports that function as "different".
 *
 * So the two functions containing a `bl` were also measured ON THEIR OWN (in a
 * temporary file, linked at their own ROM address):
 *     FUN_080315f8  -> BYTE-MATCHING on its own (30/30)
 *     FUN_08031bf4  -> BYTE-MATCHING on its own (28/28)
 * In this file both show a difference of just 4 bytes, and every differing
 * byte is part of a `bl` instruction's relative offset field; the instruction
 * sequence, the register allocation and the constants are the same as the
 * ROM's (verified with diff_function.py).  Because FUN_08030b50 has the
 * smallest address it was put FIRST in the source; that way it lands on the
 * base and its `bl`s are encoded correctly.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/band_a_317f0.c
 */

#include "gba_types.h"


/* --- the 0x020307F0 cell grid (the same as in src/core/mark_area_cells.c) - */
typedef struct CellGrid {
    s32 originY;                /* 0x00 */
    s32 originX;                /* 0x04 */
    u8  cells[1];               /* 0x08 -- row stride 0x10 */
} CellGrid;

extern CellGrid gRam020307F0;

/* --- an element of the 24 x 180-byte slot array at gRam02025810 +0x4C
 *     (src/world/release_slot.c) ---------------------------------------- */
typedef struct Held {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} Held;

/* A fixed-point position: both axes are shifted right by 22 bits to become a
 * cell (see FUN_08031534). */
typedef struct Position {
    s32 x;                      /* 0x00 */
    s32 y;                      /* 0x04 */
} Position;

extern u32  GetTextString(u32 index);
extern void FUN_0802af40(u32 word, u32 kind);
extern s32  FUN_080309a8(Held *held);
extern u32  ReleaseSlot(u32 index);
extern u8  *FUN_0806de10(u8 *dest, const u8 *src, u32 size);

/* ======================================================================= */
/* 0x080317F0 -- 84 bytes -- BYTE-MATCHING
 *
 * Packs an 8-bits-per-pixel source into 4-bits-per-pixel tile rows: on each
 * round it reads eight bytes from the source and writes two halfwords, then
 * advances the source by 40 bytes (row stride 48, 48 rounds, 192 bytes to the
 * destination).
 *
 * TWO MEASURED DETAILS:
 *  - The eighth `*src++` increment DOES NOT APPEAR in the ROM; there is an
 *    `adds r4,#41` at the end instead.  The reason: the eighth `src++` in the
 *    source merges with `src += 40`.  So the stride 48 = 8 reads + a 40-byte
 *    skip; there is NO asymmetric spelling (seven `*src++` + one `*src`) in
 *    the source.
 *  - The register allocation only becomes the ROM's with TWO SEPARATE SETS OF
 *    TEMPORARIES (rule 54).  A single set (using p0..p3 in both blocks) put
 *    src in r2 and the accumulator in r1: 31/42 instructions different.
 *    Also tried and rejected: the `for (i = 0; i < 48; i++)` form (35 bytes
 *    off; an ascending counter produces masking and a reversed loop test).
 *
 * The first five parameters are unused; because the source and destination
 * pointers are read from the stack (sp+20 / sp+24), the signature must have
 * SEVEN parameters. */
void FUN_080317f0(u32 a, u32 b, u32 c, u32 d, u32 e, u16 *dst, const u8 *src)
{
    s32 i;
    u32 p0, p1, p2, p3;
    u32 q0, q1, q2, q3;

    i = 48;
    do {
        p0 = *src++;
        p1 = *src++;
        p2 = *src++;
        p3 = *src++;
        *dst++ = p0 | (p1 << 4) | (p2 << 8) | (p3 << 12);

        q0 = *src++;
        q1 = *src++;
        q2 = *src++;
        q3 = *src++;
        *dst++ = q0 | (q1 << 4) | (q2 << 8) | (q3 << 12);

        src += 40;
        i--;
    } while (i != 0);
}


/* ======================================================================= */
/* THE FOUR FUNCTIONS NOT WRITTEN -- the reason and the structure read from
 * the ROM
 * ======================================================================= */

