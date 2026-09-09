/* Test state == 2 — 0x0805148C-0x0805149F
 *
 * Compare +0x0C in the 0x020303D0 block with 2 and return 1/0.
 * Found with tools/find_predicates.py.
 *
 * PREVIOUSLY BLOCKED: the map recorded the address as gRecordIndex, extern
 * u32, but the ROM uses it as a BASE and reads +0x0C. A structure view solved
 * this: index at +0 is RecordBlock.index; state at +0x0C is RecordBlock.state.
 * The symbol name and layout stayed unchanged, preserving record_table.c.
 *
 * Keep RecordBlock IDENTICAL to src/misc/record_table.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/is_state_two.c
 */

#include "gba_types.h"

#define STATE_READY 2

typedef struct RecordBlock {
    u32 index;                  /* +0x00 */
    u8  pad04[8];
    u32 state;                  /* +0x0C */
} RecordBlock;

extern RecordBlock gRecordIndex;

/* 0x0805148C */
u32 IsStateReady(void)
{
    if (gRecordIndex.state == STATE_READY)
        return 1;
    return 0;
}
