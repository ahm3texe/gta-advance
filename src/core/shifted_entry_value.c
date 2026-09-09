/* Read the selected entry's value, shifted — 0x0800CA78-0x0800CA97
 *
 * The object's +0x06 selects an entry in an 8-byte-stride table that starts at
 * its own +0x0C; the entry's signed halfword is shifted left by the count at
 * +0x04. The sentinel 0x7777 in +0x06 means "no entry" and yields 0.
 *
 * The ROM copies the argument with `adds r1, r0, #0` and works from the copy
 * throughout, so the parameter is taken into a local (rule 37). The sentinel
 * itself does not fit a Thumb immediate and comes from the literal pool.
 *
 * Rule 49: the `return 0` body sits at the END, reached by a forward `beq`,
 * with the value path falling through.
 *
 * Rule 70: the sentinel is taken into a local so its pool load lands BEFORE the
 * `ldrh` of the index, as the ROM has it. Compared inline against the constant,
 * the load moves to the point of use, after the field read -- one instruction
 * out of place. This is the third function where routing a value through a
 * local pulls its pool load earlier.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/shifted_entry_value.c
 */

#include "gba_types.h"

#define ENTRY_NONE  0x7777

typedef struct ValueEntry {
    s16 value;                  /* +0x00 */
    u8  pad02[6];
} ValueEntry;

typedef struct ValueTable {
    u8         pad00[4];
    u16        shift;           /* +0x04 */
    u16        index;           /* +0x06 */
    u8         pad08[4];
    ValueEntry entries[1];      /* +0x0C, 8-byte stride */
} ValueTable;

/* 0x0800CA78 */
s32 GetShiftedEntryValue(ValueTable *table)
{
    ValueTable *t = table;
    u32 none = ENTRY_NONE;
    u32 index = t->index;

    if (index == none)
        return 0;
    return t->entries[index].value << t->shift;
}
