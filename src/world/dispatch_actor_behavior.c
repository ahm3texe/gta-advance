/* The actor behaviour dispatcher — 0x08017C28-0x08017D77 (336 bytes)
 *
 * STATUS: 140/149 instructions, A NEAR MISS (does not match).  The size is
 * right.
 *
 * THE DISPATCHER of the 0x08017E3C-0x08018B73 family
 * (src/world/actor_behavior_steps.c): if the +0x1C byte of the slot selected
 * by SelectSlotCD is 0..11 it goes to the corresponding step through a
 * 12-entry jump table (StepActorBehaviorB3 for >11 and for 0).  Before that,
 * any of three entries (+0xB0, +0xA7, +0x8D) that is not 0xFF is released with
 * ClearEntry and set to 0xFF; if GetTileFieldA == 4, bit 0x20 is set at +0xA8,
 * otherwise cleared; if 0x20 is set it returns via RequestActorAction(96,4,2)
 * or (31,2,2) according to the owner's +0x18; and it returns if the +0x30 mode
 * of the entity's +0x18 record is 4.
 *
 * MEASURED: the case order of the table is 0,1,2,3,5,4,6,...,11 (the ROM's
 * body layout has 142 before 155) and `default:` shares case 0's body.
 *
 * THE REMAINING 9 INSTRUCTIONS, ONE MECHANISM: on the 0x20 set/clear branch
 * the ROM emits `orrs r0,r2` and `movs r0,#33 / negs / ands r0,r2` -- that is,
 * the AND/OR's DESTINATION is the MASK's register and the mask is 32-bit -33;
 * then a single shared `strb r0,[r1]`.  Here the destination is the value's
 * register (`orrs r2,r0`).  Tried (all measured): a ternary + a literal mask
 * (40), 32-bit mask locals + a ternary (9, THE BEST, used below), a two-step
 * `v = M; v = v & f` with separate stores (46), the same with a shared store
 * (20), selecting the mask first and applying it afterwards (51), the
 * (s32)/(u32)/-33 literal spellings (40), reading the field into a local first
 * (52).  The rule from band_b (ReleaseActorAndSlot) does not hold inside a
 * ternary here.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/dispatch_actor_behavior.c
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
