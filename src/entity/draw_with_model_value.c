/* Draw with the model's value, or none — 0x080383B8-0x080383E7
 *
 * The value comes from the +0x38 field of the record at +0x1C. Kind 4, a null
 * record and the 0x7FFF sentinel all end at the same place: 0 is passed
 * instead. The sentinel is the same "nothing" marker as in
 * src/script/cmd_call_unless_sentinel.c and comes through the literal pool for
 * the same reason.
 *
 * The kind test branches straight to the shared zero, so the sentinel
 * comparison it skips would have been redundant for it.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entity/draw_with_model_value.c
 */

#include "gba_types.h"

#define KIND_NONE  4
#define NO_VALUE   0x7FFF

typedef struct ValueRecord {
    u8   pad00[0x38];
    u16 *value;                 /* +0x38 */
} ValueRecord;

typedef struct DrawEntity {
    u8           pad00[8];
    u8           kind;          /* +0x08 */
    u8           pad09[11];
    u32          handle;        /* +0x14 */
    u8           pad18[4];
    ValueRecord *record;        /* +0x1C */
} DrawEntity;

typedef struct DrawArgs {
    u8  pad00[22];
    u16 index;                  /* +0x16 */
} DrawArgs;

extern void FUN_0801a354(u32 handle, u16 index, u16 value, u32 flags);

/* 0x080383B8 */
void FUN_080383b8(DrawEntity *entity, DrawArgs *args)
{
    u16 *slot;
    u16 value;

    if (entity->kind == KIND_NONE) goto none;
    slot = entity->record->value;
    value = 0;
    if (slot != 0)
        value = *slot;
    if (value != NO_VALUE) goto have;
none:
    value = 0;
have:
    FUN_0801a354(entity->handle, args->index, value, 0);
}
