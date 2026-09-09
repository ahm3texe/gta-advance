/* Attach the actor to a step routine — 0x0803F9A0-0x0803F9CB
 *
 * Sets bit 23 and clears bit 24 of the record's +0x0C flags in one pass, puts
 * FUN_0803D4E8 in the +0x08 step, stores the argument at +0x2C and lowers the
 * +0x81 flag. It is the counterpart of src/actor/detach_actor.c.
 *
 * The step's pool word has bit 0 set: it is a Thumb function pointer, so the
 * `__thumb`-suffixed symbol is what resolves to the address | 1
 * (tools/agbcc_build.py).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/actor/attach_actor_step.c
 */

#include "gba_types.h"

#define ATTACHED  (128 << 16)   /* 0x800000 */
#define RUNNING   (128 << 17)   /* 0x1000000 */

typedef struct ActorRecord {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} ActorRecord;

typedef struct Actor {
    ActorRecord *record;        /* +0x00 */
    ActorRecord *other;         /* +0x04 */
    u32          step;          /* +0x08 */
    u8           pad0C[0x18];
    u32          handle;        /* +0x24 */
    u8           pad28[4];
    u32          arg;           /* +0x2C */
    u8           pad30[2];
    u16          timer;         /* +0x32 */
    u8           pad34[76];
    u8           flag80;        /* +0x80 */
    u8           flag81;        /* +0x81 */
    u8           pad82;
    u8           flag83;        /* +0x83 */
    u32          w84;           /* +0x84 */
    u32          w88;           /* +0x88 */
} Actor;

extern void FUN_0803d4e8__thumb(Actor *actor);

/* 0x0803F9A0 */
void FUN_0803f9a0(Actor *actor, u32 arg)
{
    ActorRecord *record = actor->record;
    u32 flags;

    flags = record->flags;
    flags |= ATTACHED;
    flags &= ~RUNNING;
    record->flags = flags;
    actor->step = (u32)FUN_0803d4e8__thumb;
    actor->arg = arg;
    actor->flag81 = 0;
}
