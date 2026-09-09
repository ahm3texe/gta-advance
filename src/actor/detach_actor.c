/* Detach the actor — 0x0803F7D4-0x0803F7F7
 *
 * Clears bit 23 of the record's +0x0C flags, drops the +0x08 step and raises
 * the +0x81 flag. A null argument does nothing.
 *
 * ~0x800000 is 0xFF7FFFFF and comes through the literal pool; neither a `movs`
 * nor a `movs`/`lsls` pair can build it.
 *
 * The +0x81 byte goes through a pointer of its own (`adds r1,r3,#0 / adds
 * r1,#129`) because Thumb's strb immediate reaches only 31.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/actor/detach_actor.c
 */

#include "gba_types.h"

#define ATTACHED  (128 << 16)   /* 0x800000 */

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

/* 0x0803F7D4 */
void FUN_0803f7d4(Actor *actor)
{
    if (actor == 0)
        return;
    actor->record->flags &= ~ATTACHED;
    actor->step = 0;
    actor->flag81 = 1;
}
