/* Aktoru firlatmayi deneme — 0x080157B8-0x08015833
 *
 * Varligin +0x08'inde 4 kurulu, +0x30 alt nesnesi ve +0x18 kaydi dolu,
 * aktorun +0xA8 baytinin alt 4 bitinde bit0 kurulu ve alt nesnenin +0x0A
 * (s16) degeri pozitifse: kare sayacinin (gRam02000224) alt 4 biti sifir
 * oldugunda +0xA6 geri sayimi bir azaltilir; sifira ulasmadan cikilir.
 * Sonra alt nesnenin +0x08'ine 0x10000, +0xA5'e 45 yazilip +0x18 kaydi
 * 0x400000 ile SubmitPack'e verilir.
 *
 * IKI OLCUM: +0xA8'in alt 4 biti BITFIELD (`u8 low : 4`) -- ROM
 * `lsls #28 / lsrs #28` (kural 61). Alt nesnede +0x08 u16 ve +0x0A s16
 * ayri alanlar; 0x10000 yazimi ise 32 bit (`*(u32 *)&sub->h08`).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/try_launch_actor.c
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
