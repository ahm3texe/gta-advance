/* Arm the actor's timer — 0x0803F880-0x0803F89B
 *
 * Sets the +0x32 timer to 300, raises the +0x80 flag, and ADDS 0x2000000 to the
 * +0x0C flags of the record at +0x04. The add is not an or: the ROM has
 * `adds r0,r0,r2`, so the field is a counter in that bit position rather than
 * a flag.
 *
 * 300 is `movs #150 / lsls #1` and 0x2000000 is `movs #128 / lsls #18`.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/actor/arm_actor_timer.c
 */

#include "gba_types.h"

#define ARM_TIMER  (150 << 1)   /* 300 */
#define ARM_STEP   (128 << 18)  /* 0x02000000 */

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

/* 0x0803F880 */
void FUN_0803f880(Actor *actor)
{
    actor->timer = ARM_TIMER;
    actor->flag80 = 1;
    actor->other->flags += ARM_STEP;
}
