/* Notify the actor unless the flags say otherwise — 0x08038420-0x0803843D
 *
 * The +0x0C flags are tested against 0x440, two bits at once, built as
 * `movs #136 / lsls #3`. A record at +0x1C must also be present.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entity/notify_unless_masked.c
 */

#include "gba_types.h"

#define BLOCKING  (136 << 3)    /* 0x440 */

typedef struct NotifyRecord NotifyRecord;

typedef struct NotifyEntity {
    u8            pad00[12];
    u32           flags;        /* +0x0C */
    u8            pad10[12];
    NotifyRecord *record;       /* +0x1C */
} NotifyEntity;

extern void NotifyActor(NotifyRecord *record);

/* 0x08038420 */
void FUN_08038420(NotifyEntity *entity)
{
    if ((entity->flags & BLOCKING) != 0)
        return;
    if (entity->record == 0)
        return;
    NotifyActor(entity->record);
}
