/* The scoring step that closes a tracking round — 0x08064F24-0x0806512F, 524 bytes
 *
 * It works on the SAME actor structure as src/world/actor_tracking.c:
 * ResetActorTracking (0x08065378) starts the round, AccumulateActorMotion
 * (0x080653D4) grows the accumulations every frame, and this function closes
 * the round. In order:
 *
 *   1. Reads and CLEARS the +0x25 marker in the owner's player slot.
 *      If the marker is set the round is scored, otherwise nothing happens.
 *   2. If the actor's direction byte (+0x138) is 0..3, the distance covered is
 *      measured signed on that axis ((diff * 10) >> 6); if the direction byte
 *      is invalid (0xFF once the round is closed), the absolute difference on
 *      the x axis >> 11 is used.
 *   3. If the ground value (+0x60) is below the starting z, the largest drop
 *      (+0x128) is decreased by the difference between them.
 *   4. Distance, drop and the three speed accumulations (>>10) are handed to
 *      the weighted-sum helper (FUN_080627ec).
 *   5. If the distance is positive, the owner's counter (+0x30 -> +0x08) is
 *      lowered by 1/4, 2/4 or 3/4 of a step depending on the distance and is
 *      held at a floor of 0x10000; the 0x1000 bit is added to the owner's node
 *      (+0x2C), and if the node's sub-record is of kind 3 the same counter is
 *      pulled to the 0x10000 CEILING.
 *   6. If PlaceProbeEntries produces more than four entries the counter is
 *      pulled to the ceiling again.
 *   7. Finally the tracking is reset (line for line the same as the body of
 *      ResetActorTracking) and the measured distance is written into the
 *      slot's +0x4C field.
 *
 * MEASURED SPELLING RULES (all of them were tried in this function, all needed):
 *
 * A. The switch bodies were written in the ROM's block order: 0, 2, 1, 3
 *    (rule 61 addendum). With source order 0,1,2,3 the blocks come out reversed.
 *
 * B. In cases 2 and 1 the ROM loads the starting position FIRST.
 *    The spelling `pos.y - startPos.y` loads `pos.y` first (249/251).
 *    The spelling `-startPos.y + pos.y` gives the ROM's order. Ruled out:
 *    the unary `-(startPos.y - pos.y)` (agbcc folds it at the tree level, no
 *    difference), `10 * (...)` (no difference), `(startPos.y - pos.y) * -10`
 *    (532 bytes), `/ (1 << 6)` (548 bytes), an intermediate `diff` local in
 *    every case (221/251).
 *
 * C. The 6th argument of FUN_080627ec must be a VARIABLE. If you write `0`
 *    directly, agbcc emits the constant AFTER the stack write, so the fifth
 *    argument is not waiting in a register, the block asks for one register
 *    less, and the allocation of the WHOLE function shifts (actor drops to r4,
 *    the ROM has r6): 115/251. With a variable, all six of r0-r5 are occupied
 *    just like in the ROM. For the same reason the third/fourth/fifth
 *    arguments are locals too: written inline, the 2nd argument is computed
 *    first (89/251).
 *
 * D. NO INTERMEDIATE POINTER MAY BE USED IN THE COUNTER BLOCK. Writing a
 *    `counter = owner->counter` local in every branch gives 194/251; writing
 *    `owner->counter->` directly gives 225/251 and brings the ROM's register
 *    allocation (counter r1, ceiling r2, drop r3). Ruled out: a single shared
 *    `counter` local (192), a full update in every branch (207).
 *
 * E. THE 0x02035B10 STATUS WORD IS READ AND DISCARDED IN THE ROM: after
 *    `ldr r0,=..` / `ldr r0,[r0]` r0 is immediately overwritten. The only way
 *    to keep the dead load alive is a volatile view (in line with
 *    ram_symbols.h's "each translation unit declares its own view" rule).
 *    Also, because this read holds r0, NoOp08067370's argument is computed in
 *    r1 and copied into r0; passing the SAME value twice to the call (r0 and
 *    r1) is the only spelling that produces the ROM's `adds r0,r1,#0` copy.
 *    Ruled out: a single-argument call (250/251, one instruction short),
 *    `NoOp08067370(gRam02035B10, dist>>16)` (249), a third argument of `0`
 *    (250), the comma operator, an intermediate `scaled` local.
 *
 * MATCH: 524/524 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/band_08064f24.c
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

/* The player slot returned by SelectSlotAB; the +0x25 marker is the
 * same field as in src/world/actor_tracking.c. */
typedef struct PlayerSlot {
    u8  pad00[0x25];
    u8  marked;                         /* +0x25 */
    u8  pad26[0x4C - 0x26];
    s32 distance;                       /* +0x4C */
} PlayerSlot;

/* The SlotCounter from src/world/spawn_slot_effect.c; there only +0x08
 * was used, here the +0x04 step is read as well. */
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

/* The actor's +0x64 owner: the ActiveSlot from src/world/spawn_slot_effect.c
 * (there too the +0x30 counter is pulled to the same 0x10000 limit). */
typedef struct ActiveSlot {
    u8           pad00[0x2C];
    Node        *node;                  /* +0x2C */
    SlotCounter *counter;               /* +0x30 */
} ActiveSlot;

/* The TrackedActor from src/world/actor_tracking.c; in addition the +0x60
 * ground value and the owner's sub-structures are used. */
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
extern void  NoOp08067370(s32 value, s32 target);
extern void  FUN_080627ec(s32 dist, s32 drop, s32 rateB, s32 rateC,
                          s32 rateA, s32 rateD);
extern s32   PlaceProbeEntries(void *actor, s32 mode);

/* See the top of the file, item E: this word is read and discarded; a
 * volatile view is declared in this TU so the dead load survives. */
/* The type must be the SAME as in src/core/reset_runtime_globals.c (the
 * consistency check stops contradictory extern types for the same symbol).
 * The ROM performs the dead read here; volatile is applied at the expression
 * level -- the same pattern as src/world/link_service.c. */
extern u32 gRam02035B10;

/* 0x08064F24 */
void FUN_08064f24(TrackedActor *actor)
{
    PlayerSlot *slot;
    PlayerSlot *reset;
    ActiveSlot *owner;
    Node       *node;
    u8          marked;
    s32         dist;
    s32         diff;
    s32         drop;
    s32         rateA;
    s32         rateB;
    s32         rateC;
    s32         rateD;

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
        dist = (-actor->startPos.y + actor->pos.y) * DIST_NUM >> DIST_SHIFT;
        break;
    case 1:
        dist = (-actor->startPos.x + actor->pos.x) * DIST_NUM >> DIST_SHIFT;
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
    rateD = 0;
    FUN_080627ec(dist, actor->maxDelta, rateB, rateC, rateA, rateD);

    if (dist > 0) {
        owner = actor->owner;
        AreaFlagsNoop(marked);
        (void)*(volatile u32 *)&gRam02035B10;
        NoOp08067370(dist >> 16, dist >> 16);

        if (dist > DIST_STEP * 3 - 1)
            drop = owner->counter->step * 3 >> 2;
        else if (dist > DIST_STEP * 2 - 1)
            drop = owner->counter->step * 2 >> 2;
        else if (dist > DIST_STEP - 1)
            drop = owner->counter->step >> 2;
        else
            drop = 0;

        owner->counter->value -= drop;
        if (owner->counter->value <= COUNTER_LIMIT)
            owner->counter->value = COUNTER_LIMIT;

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
