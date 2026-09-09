/* First byte of the owner's slot record — 0x0806269C-0x080626BB
 *
 * GetOwnerSlot's answer goes straight into SelectSlotCD: the ROM does not touch
 * r0 between the two calls. The answer is the first byte of the record the slot
 * holds at +0x1C, or -1 when either the slot or the record is missing.
 *
 * Rule 49: the -1 is the rare answer and stands at the end, reached by both
 * forward branches.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/session/slot_first_byte.c
 */

#include "gba_types.h"

typedef struct SlotRecord {
    u8 first;                   /* +0x00 */
} SlotRecord;

typedef struct Slot {
    u8          pad00[0x1C];
    SlotRecord *record;         /* +0x1C */
} Slot;

extern s32   GetOwnerSlot(void);
extern Slot *SelectSlotCD(s32 which);

/* 0x0806269C */
s32 FUN_0806269c(void)
{
    Slot *slot = SelectSlotCD(GetOwnerSlot());

    if (slot == 0) goto none;
    if (slot->record == 0) goto none;
    return slot->record->first;
none:
    return -1;
}
