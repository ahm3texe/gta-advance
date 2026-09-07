/* Izleme turunu kapatip puan veren adim — 0x08064F24-0x0806512F, 524 bayt
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/band_08064f24.c
 */

#include "gba_types.h"

#define TRACK_MARKER    0xFF

#define DIST_NUM        10
#define DIST_SHIFT      6
#define PLAIN_SHIFT     11
#define ACCUM_SHIFT     10

#define DIST_STEP       (320 << 16)
#define COUNTER_LIMIT   (128 << 9)
#define NODE_FLAG       (128 << 5)
#define SUB_KIND        3
#define PROBE_LIMIT     3

typedef struct Vec3 {
    s32 x;                              /* +0x00 */
    s32 y;                              /* +0x04 */
    s32 z;                              /* +0x08 */
} Vec3;

/* SelectSlotAB'nin dondurdugu oyuncu yuvasi; src/world/actor_tracking.c
 * ayni +0x25 bayrağını kullaniyor. */
typedef struct PlayerSlot {
    u8  pad00[0x25];
    u8  marked;                         /* +0x25 */
    u8  pad26[0x4C - 0x26];
    s32 distance;                       /* +0x4C */
} PlayerSlot;

/* src/world/spawn_slot_effect.c'deki SlotCounter ile ayni yapi; burada
 * +0x04 adimi da okunuyor. */
typedef struct SlotCounter {
    u8  pad00[4];
    s32 step;                           /* +0x04 */
    s32 value;                          /* +0x08 */
} SlotCounter;

typedef struct SubNode {
    u8  pad00[0x26];
    u16 kind;                           /* +0x26 */
} SubNode;

typedef struct Node {
    u8       pad00[0x1E];
    u16      flags;                     /* +0x1E */
    u8       pad20[0x2C - 0x20];
    SubNode *sub;                       /* +0x2C */
} Node;

/* Aktorun +0x64 sahibi: src/world/spawn_slot_effect.c'deki ActiveSlot. */
typedef struct ActiveSlot {
    u8           pad00[0x2C];
    Node        *node;                  /* +0x2C */
    SlotCounter *counter;               /* +0x30 */
} ActiveSlot;

/* src/world/actor_tracking.c'deki TrackedActor; burada ek olarak +0x60
 * ve sahibin alt yapilari kullaniliyor. */
typedef struct TrackedActor {
    Vec3        pos;                    /* +0x000 */
    u8          pad0C[0x60 - 0x0C];
    s32         ground;                 /* +0x060 */
    ActiveSlot *owner;                  /* +0x064 */
    u8          pad68[0x118 - 0x68];
    Vec3        startPos;               /* +0x118 */
    s32         spare;                  /* +0x124 */
    s32         maxDelta;               /* +0x128 */
    s32         accumB;                 /* +0x12C */
    s32         accumC;                 /* +0x130 */
    s32         accumA;                 /* +0x134 */
    u8          marker;                 /* +0x138 */
} TrackedActor;

extern u32   GetOwnerSlot(void *owner);
extern void *SelectSlotAB(u32 which);
extern void  AreaFlagsNoop(u32 marked);
extern void  FUN_08067370(s32 value);
extern void  FUN_080627ec(s32 dist, s32 maxDelta, s32 a, s32 b, s32 c, s32 d);
extern s32   PlaceProbeEntries(void *actor, s32 mode);

extern u32   gRam02035B10;

/* 0x08064F24 */
void FUN_08064f24(TrackedActor *actor)
{
    PlayerSlot  *slot;
    PlayerSlot  *reset;
    ActiveSlot  *owner;
    SlotCounter *counter;
    Node        *node;
    u8           marked;
    s32          dist;
    s32          diff;
    s32          drop;
    s32          rateA;
    s32          rateB;
    s32          rateC;

    slot = (PlayerSlot *)SelectSlotAB(GetOwnerSlot(actor->owner));
    if (slot != 0) {
        marked = slot->marked;
        slot->marked = 0;
    } else {
        marked = 0;
    }

    if (marked == 0)
        return;

    switch (actor->marker) {
    case 0:
        dist = (actor->startPos.y - actor->pos.y) * DIST_NUM >> DIST_SHIFT;
        break;
    case 2:
        dist = (actor->pos.y - actor->startPos.y) * DIST_NUM >> DIST_SHIFT;
        break;
    case 1:
        dist = (actor->pos.x - actor->startPos.x) * DIST_NUM >> DIST_SHIFT;
        break;
    case 3:
        dist = (actor->startPos.x - actor->pos.x) * DIST_NUM >> DIST_SHIFT;
        break;
    default:
        diff = actor->startPos.x - actor->pos.x;
        if (diff < 0)
            diff = -diff;
        dist = diff >> PLAIN_SHIFT;
        break;
    }

    if (actor->ground < actor->startPos.z)
        actor->maxDelta -= actor->startPos.z - actor->ground;

    rateB = actor->accumB >> ACCUM_SHIFT;
    rateC = actor->accumC >> ACCUM_SHIFT;
    rateA = actor->accumA >> ACCUM_SHIFT;
    FUN_080627ec(dist, actor->maxDelta, rateB, rateC, rateA, 0);

    if (dist > 0) {
        owner = actor->owner;
        AreaFlagsNoop(marked);
        FUN_08067370(dist >> 16);

        if (dist > DIST_STEP * 3 - 1) {
            counter = owner->counter;
            drop = counter->step * 3 >> 2;
        } else if (dist > DIST_STEP * 2 - 1) {
            counter = owner->counter;
            drop = counter->step * 2 >> 2;
        } else if (dist > DIST_STEP - 1) {
            counter = owner->counter;
            drop = counter->step >> 2;
        } else {
            counter = owner->counter;
            drop = 0;
        }

        counter->value -= drop;
        if (counter->value <= COUNTER_LIMIT)
            counter->value = COUNTER_LIMIT;

        node = owner->node;
        if (node != 0) {
            node->flags |= NODE_FLAG;
            if (node->sub != 0 && node->sub->kind == SUB_KIND
                && owner->counter->value > COUNTER_LIMIT)
                owner->counter->value = COUNTER_LIMIT;
        }
    }

    if (PlaceProbeEntries(actor, 0) > PROBE_LIMIT) {
        if (actor->owner->counter->value > COUNTER_LIMIT)
            actor->owner->counter->value = COUNTER_LIMIT;
    }

    reset = (PlayerSlot *)SelectSlotAB(GetOwnerSlot(actor->owner));
    if (reset != 0)
        reset->marked = 0;

    actor->startPos = actor->pos;

    actor->maxDelta = 0;
    actor->accumB   = 0;
    actor->accumC   = 0;
    actor->accumA   = 0;
    actor->spare    = 0;

    actor->marker   = TRACK_MARKER;

    if (slot != 0)
        slot->distance = dist;
}
