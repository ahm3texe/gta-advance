/* Updating the actor's slot entry — 0x08015834-0x0801599D (362 bytes)
 *
 * STATUS: 150/167 instructions, A NEAR MISS (does not match).  The size is
 * right.
 *
 * If the entity kind is 35 it returns via FUN_080284d0 + (when bB1 != 0xFF)
 * FUN_08027ae8.  Otherwise the slot is taken (GetOwnerSlot) and the object and
 * record are selected with SelectSlotAB/CD.  If 0x20 is NOT set at the
 * record's +0xA8 and the +0x04 kind is NOT in the ten-value set
 * (56,32,149,150,151,16,15,12,13,31):
 *   if bit 4 is set at the entity's +0x08 and gRom08342A50[kind] is not
 *   0x7FFF, then +0x89 = value+8 and, according to IsEntryActive,
 *   FUN_08027548 / FUN_080276ac; otherwise gRom08342A14[kind] with
 *   FUN_08027150 / FUN_080272c8 (with ReleaseEntry first if the +0x0C last
 *   kind changed).  In every other case, ReleaseEntry(self, slot-1).
 *
 * THE REMAINING 17 INSTRUCTIONS, TWO MECHANISMS:
 *  1. The ROM has TWO copies of ReleaseEntry(slot-1): one right after the then
 *     branch (0x080158E0; both the precondition chain AND v==0x7FFF fall into
 *     it), the other at the end of the function (0x0801598A; only the else
 *     branch's w==0x7FFF).  Here the chain goes to the last one (11 branches).
 *     Tried: two separate ifs with an `ok` local (126), two nested releases
 *     (122), inverting the chain with an early release (121, the chain is
 *     emitted twice), a `goto release` into the block (122).  In the source the
 *     chain's failure and the then branch probably flow into the same
 *     statement; the form was not found.
 *  2. The v/index register roles (in the ROM index*4 stays in r1 and v in r2;
 *     the other way round here); an `i` local was tried (worse).
 * Decisions measured as correct: the second parameter is unused (r1 = the
 * entity goes to FUN_080284d0); the v==0x7FFF case must return from the then
 * branch with ITS OWN release (113 -> 150).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/update_actor_slot_entry.c
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
