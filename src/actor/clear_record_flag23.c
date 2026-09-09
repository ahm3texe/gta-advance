/* Clear bit 23 of the record's flags — 0x0803F9CC-0x0803F9DB
 *
 * The mask is 0xFE7FFFFF, which clears bits 23 AND 24 together;
 * src/actor/detach_actor.c clears only bit 23 with 0xFF7FFFFF, so the two are
 * not the same operation.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/actor/clear_record_flag23.c
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

/* 0x0803F9CC */
void FUN_0803f9cc(Actor *actor)
{
    actor->record->flags &= ~(ATTACHED | RUNNING);
}
