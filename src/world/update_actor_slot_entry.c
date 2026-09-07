/* Aktorun yuva girisini guncelleme — 0x08015834-0x0801599D (362 bayt)
 *
 * DURUM: 150/167 komut, YAKIN ISKA (eslesmiyor). Boyut tutuyor.
 *
 * Varlik turu 35 ise FUN_080284d0 + (bB1 != 0xFF) FUN_08027ae8 ile cikar.
 * Degilse yuva (GetOwnerSlot) alinip SelectSlotAB/CD ile nesne ve kayit
 * secilir. Kaydin +0xA8'inde 0x20 kurulu DEGILSE ve +0x04 turu on
 * degerlik kumede (56,32,149,150,151,16,15,12,13,31) DEGILSE:
 *   varligin +0x08'inde 4 kuruluysa gRom08342A50[tur] 0x7FFF degilken
 *   +0x89 = deger+8 ve IsEntryActive'e gore FUN_08027548 / FUN_080276ac;
 *   degilse gRom08342A14[tur] ile FUN_08027150 / FUN_080272c8 (once
 *   +0x0C son tur degistiyse ReleaseEntry). Diger her durumda
 *   ReleaseEntry(self, yuva-1).
 *
 * KALAN 17 KOMUT, IKI MEKANIZMA:
 *  1. ROM'da ReleaseEntry(yuva-1)'in IKI kopyasi var: biri then-dalinin
 *     hemen ardinda (0x080158E0; on kosul zinciri VE v==0x7FFF oraya
 *     dusuyor), digeri fonksiyon sonunda (0x0801598A; yalnizca else
 *     dalinin w==0x7FFF'i). Bende zincir sondakine gidiyor (11 dal).
 *     Denenen: `ok` yereliyle iki ayri if (126), ic ice iki release
 *     (122), zincirin tersini alip erken release (121, zincir iki kez
 *     uretiliyor), bloga `goto release` (122). Kaynakta muhtemelen zincir
 *     basarisizligi ile then-dali ayni deyime akiyor; bicimi bulunamadi.
 *  2. v/indis yazmac rolleri (ROM indis*4 r1'de kaliyor, v r2; bende
 *     tersi); `i` yereli denendi (daha kotu).
 * Olculen dogru kararlar: ikinci parametre kullanilmiyor (FUN_080284d0'a
 * r1 = varlik gidiyor); v==0x7FFF then-dalinda KENDI release'iyle
 * donmeli (113 -> 150).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/update_actor_slot_entry.c
 */

#include "gba_types.h"
#define KIND_35     35
#define NONE_7FFF   0x7FFF
#define FLAG_ONTILE 0x20
typedef struct Entity { u8 pad00[8]; u8 flags8; u8 kind; } Entity;
typedef struct Rec { u8 pad00[4]; u16 h04; u8 pad06[0xA2]; u8 flagsA8; } Rec;
typedef struct Obj { u8 pad00[0x1C]; Rec *rec; } Obj;
typedef struct SlotA { Obj *obj; } SlotA;
typedef struct SlotB { u8 pad00[12]; u32 lastKind; u8 pad10[12]; u8 *kind; } SlotB;
typedef struct Actor {
    u8 pad00[0x30]; Entity *entity; u8 pad34[0x55]; u8 b89; u8 pad8a[0x27]; u8 bB1; u8 padb2[2]; u32 unkB4;
} Actor;
extern const u32 gRom08342A50[];
extern const u32 gRom08342A14[];
extern u32    GetOwnerSlot(Entity *entity);
extern SlotA *SelectSlotAB(u32 which);
extern SlotB *SelectSlotCD(u32 which);
extern void   FUN_080284d0(Actor *self, u32 a, u32 b, u32 c);
extern void   FUN_08027ae8(Actor *self, Entity *entity);
extern u32    IsEntryActive(u32 slot);
extern void   ReleaseEntry(Actor *self, s32 idx);
extern void   FUN_08027548(Actor *self, Entity *entity, u32 v, u32 arg, u32 slot);
extern void   FUN_080276ac(Actor *self, Entity *entity, u32 arg, u32 slot);
extern void   FUN_08027150(Actor *self, Entity *entity, u32 v, u32 arg, u32 slot);
extern void   FUN_080272c8(Actor *self, Entity *entity, u32 arg, u32 slot);
void UpdateActorSlotEntry(Actor *self, u32 unused, u32 arg)
{
    Entity *ent; u32 slot; SlotA *ab; SlotB *cd; Rec *rec; u32 v; u32 w;
    ent = self->entity;
    if (ent->kind == KIND_35) {
        FUN_080284d0(self, (u32)ent, arg, self->unkB4);
        if (self->bB1 != 0xFF)
            FUN_08027ae8(self, self->entity);
        return;
    }
    slot = GetOwnerSlot(ent);
    if (slot == 0)
        return;
    ab = SelectSlotAB(slot);
    cd = SelectSlotCD(slot);
    rec = ab->obj->rec;
    if (!(FLAG_ONTILE & rec->flagsA8)
     && rec->h04 != 56 && rec->h04 != 32 && rec->h04 != 149 && rec->h04 != 150
     && rec->h04 != 151 && rec->h04 != 16 && rec->h04 != 15 && rec->h04 != 12
     && rec->h04 != 13 && rec->h04 != 31) {
        if (4 & self->entity->flags8) {
            v = gRom08342A50[*cd->kind];
            if (v == NONE_7FFF) {
                ReleaseEntry(self, slot - 1);
                return;
            }
            w = gRom08342A14[*cd->kind];
            self->b89 = v + 8;
            if (IsEntryActive(slot) == 0)
                FUN_08027548(self, self->entity, w, arg, slot);
            else
                FUN_080276ac(self, self->entity, arg, slot);
            return;
        }
        w = gRom08342A14[*cd->kind];
        if (cd->lastKind != *cd->kind) {
            ReleaseEntry(self, slot - 1);
            cd->lastKind = *cd->kind;
        }
        if (w != NONE_7FFF) {
            if (IsEntryActive(slot) == 0)
                FUN_08027150(self, self->entity, w, arg, slot);
            else
                FUN_080272c8(self, self->entity, arg, slot);
            return;
        }
    }
    ReleaseEntry(self, slot - 1);
}
