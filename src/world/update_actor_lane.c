/* Updating the actor's lane field — 0x080159A0-0x08015A83
 *
 * Every 4 frames (gRam02000224+5 & 3 == 0): if the entity's +0x1C owner
 * exists, the function returns when that owner's +0x3C object is missing and
 * otherwise takes its +0x27 ownership byte; with no owner it proceeds.  It
 * returns if bit 4 is not set at +0x08.  If there is a slot (GetOwnerSlot ->
 * SelectSlotAB), `has` is whether its +0x58 is non-zero.  It returns if +0x32
 * of the +0x14 record is set; and it returns if +0x31 is zero and `has` is
 * zero.  If the +0x18 of the +0x18 body, >> 15, is positive, it is multiplied
 * by gFrameDelay (or by 1 when that is zero and a slot exists) and added to
 * the bit 6-7 field of +0x8A; if that field is > 1, FUN_08026e04 is called
 * twice (0 and 1), otherwise the field is cleared when there is no slot.
 *
 * THREE MEASUREMENTS: `has = (slot != 0 && slot->w58 != 0)` must be written as
 * a SINGLE EXPRESSION; spelled as `if (slot) has = w58 != 0`, agbcc emits
 * branching code, while the ROM is branchless `negs/orrs/lsrs #31`
 * (`has |= ...` is equivalent).  The owner test must go through
 * `self->entity->owner` first and the `ent` local must be taken AFTERWARDS
 * (the ROM loads the entity into a temporary r0 first and copies it to r2
 * after the test).  gFrameDelay is the absolute macro `(*(u32 *)0x03000000)`
 * (rule 65).  The 2-bit field is a bitfield (`u8 lane : 2`); the compiler
 * generates the `+=` read-modify-write.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/update_actor_lane.c
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
