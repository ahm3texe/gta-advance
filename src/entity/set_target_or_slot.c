/* Store the target, by kind — 0x08038060-0x08038081
 *
 * Kind 2 routes the target through FUN_080387E8 together with the owner's slot;
 * every other kind writes it straight into the +0x08 word of the record the
 * entity holds at +0x30.
 *
 * Rule 71: the call is the arm that falls through, so it is the `then` arm.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entity/set_target_or_slot.c
 */

#include "gba_types.h"

#define KIND_SLOT  2

typedef struct TargetRecord {
    u8  pad00[8];
    u32 target;                 /* +0x08 */
} TargetRecord;

typedef struct KindEntity {
    u8            pad00[8];
    u8            kind;         /* +0x08 */
    u8            pad09[0x27];
    TargetRecord *record;       /* +0x30 */
} KindEntity;

extern s32  GetOwnerSlot(void);
extern void FUN_080387e8(u32 target, s32 slot);

/* 0x08038060 */
void FUN_08038060(KindEntity *entity, u32 target)
{
    if (entity->kind == KIND_SLOT)
        FUN_080387e8(target, GetOwnerSlot());
    else
        entity->record->target = target;
}
