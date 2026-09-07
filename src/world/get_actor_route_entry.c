/* Aktorun rota adimini alma — 0x080193C4-0x0801944D (+2 dolgu)
 *
 * Yuva yoksa 0. Aktorun +0x08'inde 4 kuruluysa kaydin +0xB4'u dogrudan.
 * Degilse kaydin +0x89 adimi 8'den kucukse 0; yoksa SelectSlotCD'nin
 * +0x1C tur baytiyla gRom08342A14'ten rota kumesi indisi alinip
 * gRom08BD3448.slots[..] kumesinden (adim-8). rota secilir (kume sayisini
 * asarsa 0); yuvanin +0xB0 indisi rotanin uzunlugunu asarsa son adim.
 *
 * OLCULEN: kume secimi IKI DEYIME bolunmeli (`k = tablo[tur]; slots =
 * kok.slots; set = slots[k]`); tek ifadede yazilinca agbcc kok adresini
 * tablo yuklemesinden once cekiyor ve havuz sirasi ters donuyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/get_actor_route_entry.c
 */

#include "gba_types.h"
typedef struct Route { u8 count; u8 pad01[3]; u32 *steps; } Route;
typedef struct RouteSet { s32 count; Route **routes; } RouteSet;
typedef struct RomRoot { u32 pad00; RouteSet **slots; } RomRoot;
typedef struct Rec { u8 pad00[0x89]; u8 step; u8 pad8a[0x2A]; u32 unkB4; } Rec;
typedef struct SlotB { u8 pad00[28]; u8 *kind; u8 pad20[0x90]; u16 idx; } SlotB;
typedef struct Actor { u8 pad00[8]; u8 flags8; u8 pad09[19]; Rec *rec; } Actor;
extern RomRoot gRom08BD3448;
extern const u32 gRom08342A14[];
extern u32    GetOwnerSlot(Actor *actor);
extern SlotB *SelectSlotCD(u32 which);
u32 GetActorRouteEntry(Actor *actor)
{
    u32 slot; SlotB *cd; RouteSet *set; Route *route; u32 step; s32 idx; u32 k; RouteSet **slots;
    slot = GetOwnerSlot(actor);
    if (slot == 0)
        return 0;
    if (4 & actor->flags8)
        return actor->rec->unkB4;
    if (actor->rec->step <= 7)
        return 0;
    cd = SelectSlotCD(slot);
    k = gRom08342A14[*cd->kind];
    slots = gRom08BD3448.slots;
    set = slots[k];
    step = actor->rec->step;
    if ((s32)(step - 8) >= set->count)
        return 0;
    route = set->routes[step - 8];
    idx = cd->idx;
    if (idx >= route->count)
        idx = route->count - 1;
    return route->steps[idx];
}
