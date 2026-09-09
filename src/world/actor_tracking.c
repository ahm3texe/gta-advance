/* Actor motion tracking fields — 0x08065378-0x0806543B
 *
 * The first function initializes tracking: clear the owner's slot marker, copy
 * the initial position to +0x118 and zero four accumulators. The second records
 * the maximum drop and accumulates scaled absolute velocities on all three axes
 * at each step.
 *
 * The position copy is 12 bytes; agbcc emits a single ldmia/stmia pair.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/actor_tracking.c
 */

#include "gba_types.h"

#define TRACK_MARKER 0xFF

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct TrackedActor {
    Vec3  pos;                          /* +0x000 */
    u8    pad0C[0x14 - 0x0C];
    s32   velA;                         /* +0x014 */
    u8    pad18[0x3C - 0x18];
    s32   velB;                         /* +0x03C */
    u8    pad40[0x48 - 0x40];
    s32   velC;                         /* +0x048 */
    u8    pad4C[0x64 - 0x4C];
    void *owner;                        /* +0x064 */
    u8    pad68[0x118 - 0x68];
    Vec3  startPos;                     /* +0x118 (z field at +0x120) */
    s32   spare;                        /* +0x124 */
    s32   maxDelta;                     /* +0x128 */
    s32   accumB;                       /* +0x12C */
    s32   accumC;                       /* +0x130 */
    s32   accumA;                       /* +0x134 */
    u8    marker;                       /* +0x138 */
} TrackedActor;

typedef struct PlayerSlot {
    u8 pad00[0x25];
    u8 marked;                          /* +0x25 */
} PlayerSlot;

extern u32   GetOwnerSlot(void *entity);
extern void *SelectSlotAB(u32 which);

/* 0x08065378 */
void ResetActorTracking(TrackedActor *actor)
{
    PlayerSlot *slot;

    slot = (PlayerSlot *)SelectSlotAB(GetOwnerSlot(actor->owner));
    if (slot != 0)
        slot->marked = 0;

    actor->startPos = actor->pos;

    actor->maxDelta = 0;
    actor->accumB   = 0;
    actor->accumC   = 0;
    actor->accumA   = 0;
    actor->spare    = 0;

    actor->marker   = TRACK_MARKER;
}

/* 0x080653D4 */
void AccumulateActorMotion(TrackedActor *actor, s32 scale)
{
    s32 delta;

    /* The measurement is the z displacement from the starting position:
       the ROM reads +0x120, which is startPos.z itself. */
    delta = actor->pos.z - actor->startPos.z;
    if (delta >= actor->maxDelta)
        actor->maxDelta = delta;

    delta = actor->velB;
    if (delta < 0)
        delta = -delta;
    actor->accumB += delta * scale;

    delta = actor->velC;
    if (delta < 0)
        delta = -delta;
    actor->accumC += delta * scale;

    delta = actor->velA;
    if (delta < 0)
        delta = -delta;
    actor->accumA += delta * scale;
}
