/* Clear the pair and the slot's +0x44, for kind 35 — 0x0801DB3C-0x0801DB61
 *
 * Only runs when the owner at +0x64 has a +0x09 kind byte of 35. It then clears
 * the +0x38 and +0x3C words and, if the owner's slot resolves, that slot's
 * +0x44 word as well -- all three from one zero register held across two calls,
 * which is what the `push {r4}` pays for.
 *
 * GetOwnerSlot's answer goes straight into SelectSlotAB: the ROM does not touch
 * r0 between the two calls.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/clear_pair_for_kind35.c
 */

#include "gba_types.h"

#define WANTED_KIND  35

typedef struct KindOwner {
    u8 pad00[9];
    u8 kind;                    /* +0x09 */
} KindOwner;

typedef struct PairBlock {
    u8         pad00[0x38];
    u32        first;           /* +0x38 */
    u32        second;          /* +0x3C */
    u8         pad40[0x24];
    KindOwner *owner;           /* +0x64 */
} PairBlock;

typedef struct SlotRecord {
    u8  pad00[0x44];
    u32 value;                  /* +0x44 */
} SlotRecord;

extern s32         GetOwnerSlot(KindOwner *owner);
extern SlotRecord *SelectSlotAB(s32 which);

/* 0x0801DB3C */
void FUN_0801db3c(PairBlock *block)
{
    KindOwner *owner = block->owner;
    SlotRecord *slot;
    u32 zero;

    if (owner->kind != WANTED_KIND)
        return;
    zero = 0;
    block->first = zero;
    block->second = zero;
    slot = SelectSlotAB(GetOwnerSlot(owner));
    if (slot == 0)
        return;
    slot->value = zero;
}
