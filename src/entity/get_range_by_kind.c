/* The range, by kind — 0x08038084-0x08038099
 *
 * Kind 2 has no range of its own and takes the default; every other kind reads
 * the +0x04 word of the record it holds at +0x30.
 *
 * Rule 71: the record read is the arm that falls through, so it is the `then`
 * arm and the test is written `!=`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entity/get_range_by_kind.c
 */

#include "gba_types.h"

#define KIND_SLOT  2

typedef struct RangeRecord {
    u8  pad00[4];
    u32 range;                  /* +0x04 */
} RangeRecord;

typedef struct KindEntity {
    u8           pad00[8];
    u8           kind;          /* +0x08 */
    u8           pad09[0x27];
    RangeRecord *record;        /* +0x30 */
} KindEntity;

extern u32 GetDefaultRange(void);

/* 0x08038084 */
u32 FUN_08038084(KindEntity *entity)
{
    u32 result;

    if (entity->kind != KIND_SLOT)
        result = entity->record->range;
    else
        result = GetDefaultRange();
    return result;
}
