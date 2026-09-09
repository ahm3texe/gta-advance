/* Historical band B: nine functions from 0x08030B34 .. 0x08031D23.
 *
 * MEASUREMENT NOTE: results from the former src/world/band_b.c were misleading.
 * agbcc_build.py links all functions consecutively from min(address), using
 * SUBALIGN(1). These nine are not contiguous in the ROM: other translation
 * units intervene. All but the first (SendTextMode1) were linked at a fixed
 * displacement from their ROM addresses, making every bl offset wrong even
 * for otherwise matching code (reported as 2/N differing bytes).
 * Each was therefore measured separately at its own ROM address:
 *   SendTextMode1 12 BYTE-MATCHING
 *   LoadHudPalettes 124 BYTE-MATCHING
 *   TriggerEvent39 12 BYTE-MATCHING
 *   GetRecordNodeById 14 BYTE-MATCHING
 *   ScaleMagnitude 132 BYTE-MATCHING
 *   ClearHudRowsAB 72 BYTE-MATCHING
 *   ReleaseActorAndSlot 64 BYTE-MATCHING
 *   BlitStripClipLeft4bpp 470 NON-MATCHING
 *   PushSlotQueueEntry 140 MISSING RAM SYMBOL
 * These are historical results; detailed notes accompany the split sources.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm (docs/COMPILER.md)
 * Verification: make c-match FILE=src/world/release_actor_and_slot.c
 */

#include "gba_types.h"
#include "gba_io.h"

/* ---- 0x08031618 — 64 bytes, BYTE-MATCHING -------------------------------
 *
 * Releases an actor: if bit 2 is set in the +0x1E flag word it clears the bit
 * and releases the object slot at +0x28, then asks FUN_08030d0c for the +0x18
 * id of the owner at +0x2C and calls ReleaseSlot unless the returned slot
 * is -1.
 *
 * THE ONE HARD POINT — THE MASK'S WIDTH AND THE AND'S DESTINATION:
 * the ROM has `ldr r0,=0xfffd / ands r0,r1 / strh r0,[r4,#30]`; the AND's
 * DESTINATION is the MASK's register.  `actor->flags = flags & ACTOR_FLAG_CLEAR;`
 * (and every variant that takes the mask into a u16 local) folds the result
 * into `flags`'s register and produces `ands r1,r0` — 2 bytes off, and it does
 * not close.  The measured solution: the mask must be a 32-BIT local and the
 * AND's destination must be THAT local.  A 16-bit local DOES NOT give the same
 * result (`ands r1,r0`); the width is decisive too.  Equivalents that give the
 * same result (all measured, all matching): a u32/s32/int local,
 * `next &= flags` or `next = next & flags`, declared at block scope or at the
 * top of the function.  Ruled out (all 2 bytes off): `flags & MASK`,
 * `MASK & flags`, a u16 mask local, `flags &= MASK`,
 * `flags & ~ACTOR_FLAG_HELD`, a temporary result variable, u32/s32 `flags`,
 * moving the mask to the top of the function, moving held first.
 *
 * The -1 comparison comes out as `movs r0,#1 / negs r0,r0 / cmp r1,r0`; that
 * is simply what the plain spelling gives, since -1 cannot be a `cmp`
 * immediate in Thumb.
 */

#define ACTOR_FLAG_HELD    2
#define ACTOR_FLAG_CLEAR   0xFFFD       /* ~ACTOR_FLAG_HELD, 16 bits */
#define SLOT_NONE          (-1)

typedef struct Obj Obj;

typedef struct Owner {
    u8  pad00[0x18];
    u16 id;                     /* +0x18 */
} Owner;

typedef struct Actor {
    u8     pad00[0x1E];
    u16    flags;               /* +0x1E */
    u8     pad20[8];
    Obj   *held;                /* +0x28 */
    Owner *owner;               /* +0x2C */
} Actor;

extern void ReleaseObjectSlot(Obj *o);
extern s32  FUN_08030d0c(u32 id);
extern u32  ReleaseSlot(u32 index);

/* 0x08031618 */
void ReleaseActorAndSlot(Actor *actor)
{
    u16 flags;
    s32 slot;

    if (actor == 0)
        return;

    flags = actor->flags;
    if (flags & ACTOR_FLAG_HELD) {
        u32 next;

        next = ACTOR_FLAG_CLEAR;
        next = next & flags;
        actor->flags = next;
        ReleaseObjectSlot(actor->held);
    }

    slot = FUN_08030d0c(actor->owner->id);
    if (slot != SLOT_NONE)
        ReleaseSlot(slot);
}

