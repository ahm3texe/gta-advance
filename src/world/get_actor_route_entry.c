/* Get an actor's route step — 0x080193C4-0x0801944D (+2 padding)
 *
 * Return 0 if there is no slot. If bit 4 at actor +0x08 is set, return record
 * +0xB4 directly. Otherwise return 0 if step +0x89 is below 8. Use SelectSlotCD's
 * type byte at +0x1C to look up a route-set index in gRom08342A14, then select
 * route (step-8) from gRom08BD3448.slots[..]; return 0 if it exceeds the set
 * count. If slot index +0xB0 exceeds the route length, use the last step.
 *
 * MEASURED: split set selection into separate statements
 * (`k = table[type]; slots = root.slots; set = slots[k]`). A single expression
 * makes agbcc load the root address before the table load, reversing pool order.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/get_actor_route_entry.c
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
