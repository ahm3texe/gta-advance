/* Trying to launch the actor — 0x080157B8-0x08015833
 *
 * If bit 4 is set at the entity's +0x08, its +0x30 sub-object and +0x18
 * record are both present, bit0 of the low 4 bits of the actor's +0xA8 byte
 * is set and the sub-object's +0x0A (s16) value is positive: whenever the
 * low 4 bits of the frame counter (gRam02000224) are zero, the +0xA6
 * countdown is decremented by one; the function returns before it reaches
 * zero.  Then 0x10000 is written to the sub-object's +0x08 and 45 to +0xA5,
 * and the +0x18 record is handed to SubmitPack with 0x400000.
 *
 * TWO MEASUREMENTS: the low 4 bits of +0xA8 are a BITFIELD (`u8 low : 4`) --
 * the ROM has `lsls #28 / lsrs #28` (rule 61).  In the sub-object, +0x08 (u16)
 * and +0x0A (s16) are separate fields; the 0x10000 store, however, is 32 bits
 * wide (`*(u32 *)&sub->h08`).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/try_launch_actor.c
 */

#include "gba_types.h"
typedef struct Sub { u8 pad00[8]; u16 h08; s16 s10; } Sub;
typedef struct Entity { u8 pad00[8]; u8 flags8; u8 pad09[15]; u8 *unk18; u8 pad1c[20]; Sub *sub; } Entity;
typedef struct Actor { u8 pad00[0x30]; Entity *entity; u8 pad34[0x71]; u8 bA5; u8 bA6; u8 pad; u8 low : 4; u8 high : 4; } Actor;
extern u32 gRam02000224;
extern void SubmitPack(u8 *p, u32 value);
void TryLaunchActor(Actor *self)
{
    Entity *ent; Sub *sub;
    ent = self->entity;
    if (!(4 & ent->flags8))
        return;
    sub = ent->sub;
    if (sub == 0)
        return;
    if (ent->unk18 == 0)
        return;
    if (!(1 & self->low))
        return;
    if (sub->s10 <= 0)
        return;
    if ((gRam02000224 & 15) == 0 && self->bA6 != 0)
        self->bA6--;
    if (self->bA6 != 0)
        return;
    *(u32 *)&self->entity->sub->h08 = 0x10000;
    self->bA5 = 45;
    SubmitPack(self->entity->unk18, 0x400000);
}
