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
 * Verification:  make c-match FILE=src/world/band_a_31534.c
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
/* 0x08031534 -- 68 bytes -- BYTE-MATCHING
 *
 * Converts a fixed-point position into the cell grid and returns whether that
 * cell is EMPTY.  The grid view is the same as in src/core/mark_area_cells.c
 * (originY +0, originX +4, cells +8, row stride 0x10).
 *
 * THREE DETAILS READ FROM THE ROM:
 *  - The column guard is a SINGLE unsigned test (`cmp #31 / bhi`): by rule 60
 *    that is the folded form of `if (column < 0 || column > 31)`, so in the
 *    source `column` is unsigned and there is one comparison.
 *  - The row guard is TWO separate signed tests (`cmp #0 / blt`,
 *    `cmp #31 / bgt`): two separate `if`s.  Joining them with `&&` on one line
 *    would fold them.
 *  - The result is materialized in a local (`movs r4,#0` even BEFORE the
 *    address computation, then `movs r4,#1`): rule 48.
 *  - The `return 0` body is AFTER the pool, at the end of the function: rule
 *    49, i.e. a `goto` to the end rather than an early `return 0`. */
u32 FUN_08031534(Position *pos)
{
    u32 column;
    s32 row;
    u32 result;

    column = (pos->x >> 22) - gRam020307F0.originX;
    row = (pos->y >> 22) - gRam020307F0.originY;
    if (column >= 0x20)
        goto none;
    if (row < 0)
        goto none;
    if (row >= 0x20)
        goto none;

    result = 0;
    if (gRam020307F0.cells[row * 0x10 + column] == 0)
        result = 1;
    return result;

none:
    return 0;
}


/* ======================================================================= */
