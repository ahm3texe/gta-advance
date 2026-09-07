/* Aktor davranis dagiticisi — 0x08017C28-0x08017D77 (336 bayt)
 *
 * DURUM: 140/149 komut, YAKIN ISKA (eslesmiyor). Boyut tutuyor.
 *
 * 0x08017E3C-0x08018B73 ailesinin (src/world/actor_behavior_steps.c)
 * DAGITICISI: SelectSlotCD ile secilen yuvanin +0x1C bayti 0..11 ise
 * 12 girisli atlama tablosuyla ilgili adima gidiyor (>11 ve 0 icin
 * StepActorBehaviorB3). Oncesinde uc giris (+0xB0, +0xA7, +0x8D) 0xFF
 * degilse ClearEntry ile birakilip 0xFF yapiliyor; GetTileFieldA == 4 ise
 * +0xA8'de 0x20 kuruluyor, degilse siliniyor; 0x20 kuruluysa sahibin
 * +0x18'ine gore RequestActorAction(96,4,2) ya da (31,2,2) ile cikiliyor;
 * varligin +0x18 kaydinin +0x30 kipi 4 ise cikiliyor.
 *
 * OLCULEN: tablo case sirasi 0,1,2,3,5,4,6,...,11 (ROM govde yerlesimi
 * 155'ten once 142) ve `default:` case 0 govdesini paylasiyor.
 *
 * KALAN 9 KOMUT, TEK MEKANIZMA: 0x20 kurma/silme dalinda ROM `orrs r0,r2`
 * ve `movs r0,#33 / negs / ands r0,r2` uretiyor -- yani AND/OR'un HEDEFI
 * MASKENIN yazmaci ve maske -33 olarak 32 bit; ardindan tek ortak
 * `strb r0,[r1]`. Bende hedef deger yazmaci (`orrs r2,r0`). Denenen
 * (hepsi olcumlu): ternary + literal maske (40), 32 bit maske yerelleri +
 * ternary (9, EN IYI, asagidaki), iki adimli `v = M; v = v & f` ayri
 * store (46), ayni ama ortak store (20), maskeyi once secip sonra
 * uygulamak (51), (s32)/(u32)/-33 literal biçimleri (40), alani once
 * yerele okumak (52). band_b'deki kural (ReleaseActorAndSlot) burada
 * ternary icinde tutmuyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/dispatch_actor_behavior.c
 */

#include "gba_types.h"
#define ENTRY_NONE   0xFF
#define FLAG_ONTILE  0x20
#define TILE_KIND_4  4
#define MODE_4       4
typedef struct Owner { u8 pad00[24]; void *unk18; } Owner;
typedef struct Entity { u8 pad00[24]; u8 *unk18; } Entity;
typedef struct SlotB { u8 pad00[28]; u8 *kind; } SlotB;
typedef struct Actor {
    Owner *owner; u8 pad04[44]; Entity *entity; u8 pad34[0x59];
    u8 entryC; u8 pad8e[0x19]; u8 entryB; u8 flagsA8; u8 pada9[7]; u8 entryA;
} Actor;
extern u32    GetOwnerSlot(Entity *entity);
extern SlotB *SelectSlotCD(u32 which);
extern void   ClearEntry(u8 index);
extern u32    GetTileFieldA(u8 *tile);
extern void   RequestActorAction(Actor *self, s32 a, s32 b, s32 c);
extern void StepActorBehaviorB3(Actor *self);
extern void StepActorBehavior130(Actor *self);
extern void StepActorBehavior133(Actor *self);
extern void StepActorBehavior149(Actor *self);
extern void StepActorBehavior142(Actor *self);
extern void StepActorBehavior155(Actor *self);
extern void StepActorBehavior136(Actor *self);
extern void StepActorBehavior139(Actor *self);
extern void StepActorBehaviorB4b(Actor *self);
extern void StepActorBehaviorB4a(Actor *self);
extern void StepActorBehavior152(Actor *self);
extern void StepActorBehavior146(Actor *self);
void DispatchActorBehavior(Actor *self)
{
    u32 which; SlotB *cd; u32 set; s32 clr;
    which = GetOwnerSlot(self->entity);
    if (which == 0)
        return;
    cd = SelectSlotCD(which);
    if (self->entryA != ENTRY_NONE) {
        ClearEntry(self->entryA);
        self->entryA = ENTRY_NONE;
    }
    if (self->entryB != ENTRY_NONE) {
        ClearEntry(self->entryB);
        self->entryB = ENTRY_NONE;
    }
    if (self->entryC != ENTRY_NONE) {
        ClearEntry(self->entryC);
        self->entryC = ENTRY_NONE;
    }
    set = FLAG_ONTILE;
    clr = ~FLAG_ONTILE;
    self->flagsA8 = (GetTileFieldA(self->entity->unk18) == TILE_KIND_4)
                  ? (set | self->flagsA8)
                  : (clr & self->flagsA8);
    if (FLAG_ONTILE & self->flagsA8) {
        if (self->owner->unk18 != 0)
            RequestActorAction(self, 96, 4, 2);
        else
            RequestActorAction(self, 31, 2, 2);
        return;
    }
    if (*(self->entity->unk18 + 0x30) == MODE_4)
        return;
    switch (*cd->kind) {
    default:
    case 0:  StepActorBehaviorB3(self);  break;
    case 1:  StepActorBehavior130(self); break;
    case 2:  StepActorBehavior133(self); break;
    case 3:  StepActorBehavior149(self); break;
    case 5:  StepActorBehavior142(self); break;
    case 4:  StepActorBehavior155(self); break;
    case 6:  StepActorBehavior136(self); break;
    case 7:  StepActorBehavior139(self); break;
    case 8:  StepActorBehaviorB4b(self); break;
    case 9:  StepActorBehaviorB4a(self); break;
    case 10: StepActorBehavior152(self); break;
    case 11: StepActorBehavior146(self); break;
    }
}
