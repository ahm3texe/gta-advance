/* Is the entry's state byte 1 — 0x08029374-0x0802938F
 *
 * Indexes the 148-byte entry table and returns 1 when the +0x2B state byte is
 * 1, 0 otherwise.
 *
 * Rule 48: the ROM builds the result in a VARIABLE rather than branching to two
 * returns. `movs r1,#0` is emitted before the field is even read, and the
 * `movs r1,#1` sits in the taken branch; the tail is a plain `adds r0,r1,#0`.
 *
 * THE ELEMENT ADDRESS MUST BE ITS OWN STATEMENT, AHEAD OF THE ZERO. Written as
 * one expression (`if (gRam020246F0[index].state == 1)`) agbcc hoists both the
 * zero and the table's pool load to the top of the function, ahead of the
 * multiply; the ROM emits them between the multiply and the field read. Taking
 * the element into a local first restores that order. Three spellings are
 * equivalent and all match: an `Entry *`, a `u8 *` with `p[43]`, and the same
 * with the declarations and assignments split.
 *
 * The Entry body is the one shared with src/world/table_entries.c and
 * src/world/step_entry_timer.c (the consistency gate requires one body per
 * symbol).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entry_state_is_one.c
 */

#include "gba_types.h"

typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01;
    u16 unk02;                  /* +0x02 */
    u32 unk04;                  /* +0x04 (the block to be released) */
    u8  pad08[0x22];
    u8  mark;                   /* +0x2A */
    u8  state;                  /* +0x2B */
    u8  pad2C[0x38];
    u8  tableIndex;             /* +0x64 */
    u8  pad65[0x27];
    s32 unk8C;                  /* +0x8C */
    u32 phase;                  /* +0x90 */
} Entry;

extern Entry gRam020246F0[20];

/* 0x08029374 */
u32 IsEntryStateOne(u32 index)
{
    Entry *e = &gRam020246F0[index];
    u32 result = 0;

    if (e->state == 1)
        result = 1;
    return result;
}
