/* Engaging the actor — 0x08055B34-0x08055B7D
 *
 * Two gates: mask 12 of the +0x0B byte must be 8, and bit 0 of GetEntityKind's
 * result must be set.  If both hold, bit 9 is set and bit 0 cleared in the
 * target's flags, SetActorEngage is called, then bit 7 is set as well and 1 is
 * returned.
 *
 * Rule 33: the constant must go into a SEPARATE result local and be updated
 * in place with `&=`.  The ROM builds the mask FIRST with `movs r0,#12`, then
 * reads the field and updates the mask in its own register; writing
 * `field & 12` keeps the result in the field's register and produces different
 * code.
 *
 * Rule 35: `pop {r1}; bx r1` -> r0 carries a return value, so the signature
 * is u32.
 *
 * THE BLOCK ORDER MATTERS: the ROM's failure block falls BETWEEN the TWO
 * checks -- the first check branches forward on failure, the second branches
 * on SUCCESS and the failure path is entered by FALLING THROUGH.  Writing two
 * separate `return 0`s produced different branch distances (bne +0x32 instead
 * of the ROM's +0x0C).  The labelled form expresses the ROM's control flow
 * directly; the same solution was used in src/world/bump_or_reset.c.
 *
 * The ROM RE-READS +0x28 AFTER the call; that is natural behaviour, since the
 * call may have changed it.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/engage_actor.c
 */

#include "gba_types.h"

#define STATE_MASK  12
#define STATE_READY 8
#define KIND_BIT    1
#define FLAG_SET    (0x80 << 2)
#define FLAG_DONE   0x80

typedef struct Target {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
    u8  pad10[20];
    u32 unk24;                  /* +0x24 */
} Target;

typedef struct Actor {
    u8      pad00[11];
    u8      state;              /* +0x0B */
    u8      pad0C[24];
    u32     unk24;              /* +0x24 */
    Target *target;             /* +0x28 */
} Actor;

extern u32  GetEntityKind(u32 arg);
extern void SetActorEngage(u32 arg);

/* 0x08055B34 */
u32 EngageActor(Actor *actor)
{
    u32 state;
    Target *target;

    state = STATE_MASK;
    state &= actor->state;
    if (state != STATE_READY)
        goto fail;
    if (GetEntityKind(actor->unk24) & KIND_BIT)
        goto engage;

fail:
    return 0;

engage:
    target = actor->target;
    target->flags = (target->flags | FLAG_SET) & ~1;
    SetActorEngage(target->unk24);

    target = actor->target;
    target->flags |= FLAG_DONE;
    return 1;
}
