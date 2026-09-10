/* Draw the owner's table entry — 0x0800DE5C-0x0800DE87
 *
 * The record hangs off the +0x00 pointer's +0x04 field. Both it and its +0x34
 * halfword must be non-zero. The entry taken from the +0x38 array is chosen by
 * bits 2 and 3 of the +0x1F byte, extracted with `lsls #30 / lsrs #28`, which
 * is a two-bit field SCALED BY FOUR -- a word index, not a byte one.
 *
 * The array base goes through a local of its own, or the +0x38 folds into the
 * load's displacement and the ROM's `adds r0,#56` on the base disappears.
 *
 * The last argument is the owner ADVANCED by four, not its +0x04 field: the ROM
 * has `adds r2,#4` on the pointer itself.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/draw_from_owner_table.c
 */

#include "gba_types.h"

#define ENTRY_OFFSET  0x38

typedef struct DrawRecord {
    u8  pad00[0x34];
    u16 count;                  /* +0x34 */
    u8  pad36[2];
    u32 entries[1];             /* +0x38 */
} DrawRecord;

typedef struct DrawHolder {
    u32         pad00;
    DrawRecord *record;         /* +0x04 */
} DrawHolder;

typedef struct DrawOwner {
    DrawHolder *holder;         /* +0x00 */
    u8          pad04[27];
    u8          select;         /* +0x1F */
} DrawOwner;

extern void FUN_0801019c(u32 entry, u16 count, void *owner);

/* 0x0800DE5C */
void FUN_0800de5c(DrawOwner *owner)
{
    DrawRecord *record = owner->holder->record;
    u8 *entries;
    u32 index;

    if (record == 0)
        return;
    if (record->count == 0)
        return;
    index = (u32)(owner->select << 30) >> 28;
    entries = (u8 *)record + ENTRY_OFFSET;
    FUN_0801019c(*(u32 *)(entries + index), record->count, (u8 *)owner + 4);
}
