/* SPLIT OFF from band_a.c.  The reason: agbcc_build.py links a translation
 * unit at min(address of the functions defined in it) and lays the functions
 * out in SOURCE ORDER.  When functions that are NOT contiguous in the ROM sit
 * in one file, everything after the first lands at the wrong address and the
 * `bl` relative offsets shift.  Those without external calls still match; the
 * two that contain calls did not.  A separate file = the right link base.
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
 * Verification:  make c-match FILE=src/world/band_a_31bf4.c
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
/* 0x08030B50 -- 16 bytes -- BYTE-MATCHING
 *
 * Looks up the record word and forwards it with mode 2.  The same pattern as
 * src/world/lookup_then_call.c (0x08030C0C); the only difference is that the
 * second argument is a constant here.
 *
 * IT MUST COME FIRST IN THE SOURCE: the file's link base is this function's
 * address (the smallest ROM address); anywhere else and the `bl` offsets
 * shift. */

/* 0x08031BF4 -- 28 bytes
 * BYTE-MATCHING ON ITS OWN (28/28, measured linked at its own ROM address).
 * IN THIS FILE it differs: 3/28 bytes -- the three differing bytes are only
 * the relative offset field of a single `bl` instruction; see the layout note
 * in the file header.
 *
 * Copies a 79-byte string into a buffer on the stack.  FUN_0806DE10
 * (0x0806DE10) was taken apart: the classic `strncpy` -- a word copy 4 bytes
 * at a time first, then byte by byte, filling the rest with zeroes, returning
 * `dest`.  Its return value is not used here.
 *
 * THE BUFFER SIZE IS 84: the ROM's frame is `sub sp,#84`.  The copied string
 * has a capacity of 80 bytes (79 bytes + a terminator), so four extra bytes
 * sit in the frame and WHAT THEY ARE CANNOT BE READ FROM THE ROM.  All four of
 * the sizes 81, 82, 83 and 84 produce exactly the same code (measured); 80
 * gives `sub sp,#80` and diverges by two bytes.  With no distinguishing
 * evidence, the round number 84 was chosen. */
void FUN_08031bf4(const u8 *src)
{
    u8 buffer[84];

    buffer[79] = 0;
    FUN_0806de10(buffer, src, 79);
}
