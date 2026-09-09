/* Release the actor's handle — 0x0803F8BC-0x0803F8DB
 *
 * The +0x24 handle is handed to FUN_0800C804 with the address of gRam020110C0,
 * the same pairing data/ram_map.csv records for ReleaseAreaNode, and then
 * cleared. A zero handle does nothing.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/actor/release_actor_handle.c
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

extern u32 gRam020110C0[];

extern void FUN_0800c804(u32 *dest, u32 value);

/* 0x0803F8BC */
void FUN_0803f8bc(Actor *actor)
{
    if (actor->handle == 0)
        return;
    FUN_0800c804(gRam020110C0, actor->handle);
    actor->handle = 0;
}
