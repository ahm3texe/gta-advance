/* Spawn a pair at the node's nudged point — 0x0805A7A0-0x0805A823
 *
 * The long form of src/script/cmd_spawn_at_offset.c: the same point, built from
 * the node's record and nudged by the two SIGNED bytes at +0x20 and +0x21, but
 * handed to FUN_0802B1E4 as two separate coordinates rather than as a struct,
 * together with the second operand and a zero.
 *
 * The struct on the stack exists only because FUN_080557AC fills it; the ROM
 * passes r0 and r1 straight from the two `str` results, so the callee takes the
 * coordinates and not the address.
 *
 * The tail is the settle check src/script/cmd_place_record.c also has: when
 * neither the +0x1358 nor the +0x1360 handle is held, FUN_08029918 runs. The
 * second offset is DERIVED from the first with `adds r2,#8` (rule 65).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_spawn_pair_at_offset.c
 */

#include "gba_types.h"

#define UNIT_SHIFT  16
#define HANDLE_A    0x1358

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

extern u32 gFrameCounterEwram;
extern u8  gRam02025810[];

extern SpawnNode *FindOrRecycleNode(s32 id);

extern void FUN_080557ac(SpawnPoint *point, u16 id);
extern void FUN_0802b1e4(s32 x, s32 y, u16 kind, u32 flags);
extern void FUN_08029918(void);

/* 0x0805A7A0 */
u32 FUN_0805a7a0(u32 a, u16 id, u16 kind)
{
    SpawnNode *node = FindOrRecycleNode(id);
    SpawnRecord *record = node->record;
    SpawnPoint point;
    u8 *base;
    u32 offset;

    if (gFrameCounterEwram == 0)
        return 0;
    FUN_080557ac(&point, record->kind->id);
    point.x += record->offsetX << UNIT_SHIFT;
    point.y += record->offsetY << UNIT_SHIFT;
    FUN_0802b1e4(point.x, point.y, kind, 0);
    base = gRam02025810;
    offset = HANDLE_A;
    if (*(u32 *)(base + offset) == 0) {
        offset += 8;
        if (*(u32 *)(base + offset) == 0)
            FUN_08029918();
    }
    return 1;
}
