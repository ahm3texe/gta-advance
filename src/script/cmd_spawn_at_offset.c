/* Spawn at the node's offset — 0x08059DEC-0x08059E47
 *
 * Builds a three-word point from the node's record, nudges it by the two SIGNED
 * bytes at +0x20 and +0x21, and hands it to FUN_08038944 with 0x800000. A
 * non-zero answer raises gRam0201627C.
 *
 * Both nudges are `x << 16` of a signed byte, so the bytes are whole units in a
 * 16-bit fractional coordinate. They are `ldrsb` through a register offset,
 * which is the only form Thumb has and also what proves the bytes are signed.
 *
 * The second byte's address is the record ADVANCED by 33 rather than a second
 * offset from the base (`adds r4,#33`), so the record pointer is reused and not
 * re-derived (rule 65).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_spawn_at_offset.c
 */

#include "gba_types.h"

#define UNIT_SHIFT  16
#define SPAWN_FLAG  (128 << 16) /* 0x800000 */

typedef struct SpawnKind {
    u16 id;                     /* +0x00 */
} SpawnKind;

typedef struct SpawnRecord {
    u8         pad00[0x0C];
    SpawnKind *kind;            /* +0x0C */
    u8         pad10[0x10];
    s8         offsetX;         /* +0x20 */
    s8         offsetY;         /* +0x21 */
} SpawnRecord;

typedef struct SpawnNode {
    u8           pad00[0x14];
    SpawnRecord *record;        /* +0x14 */
} SpawnNode;

typedef struct SpawnPoint {
    s32 x;
    s32 y;
    s32 z;
} SpawnPoint;

extern u32 gRam0201627C;

extern SpawnNode *FindOrRecycleNode(s32 id);

extern void FUN_080557ac(SpawnPoint *point, u16 id);
extern u32  FUN_08038944(const SpawnPoint *point, u32 flags);

/* 0x08059DEC */
u32 FUN_08059dec(u32 a, u16 id)
{
    SpawnNode *node = FindOrRecycleNode(id);
    SpawnRecord *record = node->record;
    SpawnPoint point;
    u32 result;

    FUN_080557ac(&point, record->kind->id);
    point.x += record->offsetX << UNIT_SHIFT;
    point.y += record->offsetY << UNIT_SHIFT;
    result = FUN_08038944(&point, SPAWN_FLAG);
    if (result != 0)
        gRam0201627C = 1;
    return result;
}
