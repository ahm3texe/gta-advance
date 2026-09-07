/* Aktor serit alanini guncelleme — 0x080159A0-0x08015A83
 *
 * Her 4 karede bir (gRam02000224+5 & 3 == 0): varligin +0x1C sahibi varsa
 * onun +0x3C nesnesi yoksa cikilir, varsa +0x27 sahiplik bayti; sahip yoksa
 * gecerli. +0x08'de 4 kurulu degilse cikilir. Yuva (GetOwnerSlot ->
 * SelectSlotAB) varsa +0x58 sifir degil mi (has). +0x14 kaydinin +0x32'si
 * kuruluysa cikilir; +0x31 sifir ve has sifirsa cikilir. +0x18 govdesinin
 * +0x18'i >> 15 pozitifse gFrameDelay ile carpilip (sifirsa ve yuva varsa 1)
 * +0x8A'nin 6-7. bit alanina eklenir; alan > 1 ise FUN_08026e04 iki kez
 * (0 ve 1), degilse yuva yoksa alan sifirlanir.
 *
 * UC OLCUM: `has = (slot != 0 && slot->w58 != 0)` TEK IFADE yazilmali;
 * `if (slot) has = w58 != 0` biciminde agbcc dalli kod uretiyor, ROM
 * dalsiz `negs/orrs/lsrs #31` (`has |= ...` de esit). Sahip testi
 * `self->entity->owner` uzerinden once, `ent` yereli SONRA alinmali (ROM
 * varligi once gecici r0'a yukleyip testten sonra r2'ye kopyaliyor).
 * gFrameDelay `(*(u32 *)0x03000000)` mutlak makro (kural 65). 2 bitlik
 * alan bitfield (`u8 lane : 2`), `+=` RMW'yi derleyici uretiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/update_actor_lane.c
 */

#include "gba_types.h"
typedef struct Obj { u8 pad00[0x27]; u8 owned; } Obj;
typedef struct Owner { u8 pad00[0x3C]; Obj *obj; } Owner;
typedef struct Det { u8 pad00[0x31]; u8 b31; u8 b32; } Det;
typedef struct Body { u8 pad00[0x18]; s32 w18; } Body;
typedef struct Entity { u8 pad00[8]; u8 flags8; u8 pad09[11]; Det *det; Body *body; Owner *owner; } Entity;
typedef struct SlotA { u8 pad00[0x58]; u32 w58; } SlotA;
typedef struct Actor { u8 pad00[0x30]; Entity *entity; u8 pad34[0x56]; u8 low : 6; u8 lane : 2; } Actor;
extern u32 gRam02000224;
#define gFrameDelay (*(u32 *)0x03000000)
extern u32    GetOwnerSlot(Entity *entity);
extern SlotA *SelectSlotAB(u32 which);
extern void   FUN_08026e04(Entity *entity, u32 arg, u32 which);
void UpdateActorLane(Actor *self, u32 unused, u32 arg)
{
    Entity *ent; Obj *obj; u32 ok; SlotA *slot; u32 has; Det *det; s32 v; s32 k;
    if (((gRam02000224 + 5) & 3) != 0)
        return;
    if (self->entity->owner != 0) {
        obj = self->entity->owner->obj;
        if (obj == 0)
            return;
        ok = obj->owned;
    } else {
        ok = 1;
    }
    if (ok == 0)
        return;
    ent = self->entity;
    if (!(4 & ent->flags8))
        return;
    slot = SelectSlotAB(GetOwnerSlot(ent));
    has = (slot != 0 && slot->w58 != 0);
    ent = self->entity;
    det = ent->det;
    if (det->b32 != 0)
        return;
    if (det->b31 == 0 && has == 0)
        return;
    v = ent->body->w18 >> 15;
    if (v <= 0)
        return;
    k = v * gFrameDelay;
    if (k <= 0 && GetOwnerSlot(ent) != 0)
        k = 1;
    self->lane += k;
    if (self->lane > 1) {
        FUN_08026e04(self->entity, arg, 0);
        FUN_08026e04(self->entity, arg, 1);
    } else if (GetOwnerSlot(self->entity) == 0) {
        self->lane = 0;
    }
}
