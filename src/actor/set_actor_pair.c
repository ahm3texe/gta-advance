/* Raise the +0x83 flag and store two words — 0x0803F89C-0x0803F8BB
 *
 * The two arguments land at +0x84 and +0x88 in the OPPOSITE order to the
 * calling convention: r2 goes to +0x84 and r1 to +0x88.
 *
 * The +0x88 store derives its address from the +0x84 one with `adds r0,#4`
 * (rule 65), and the +0x83 byte goes through a pointer for the reason
 * src/actor/detach_actor.c records: strb's immediate reaches only 31.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/actor/set_actor_pair.c
 */

#include "gba_types.h"

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

/* 0x0803F89C */
void FUN_0803f89c(Actor *actor, u32 second, u32 first)
{
    actor->flag83 = 1;
    actor->w84 = first;
    actor->w88 = second;
}
