/* Re-initializes the actor: links the embedded sub-structure and clears the
 * flags.  0x08016768, 160 bytes.
 *
 * The address of the structure embedded at the actor's OWN +0x40 is written
 * into the +0x3C field, and that structure is then set up by FUN_08014ffc.
 * The setup flag is made of two parts: bit 0x80 if the 0x140000 mask of the
 * entity's +0x0C flags is NON-ZERO, plus, under certain conditions, a
 * three-bit field extracted from +0x3E and shifted left by 9.  3 is added to
 * the result before it is passed on.
 *
 * Ghidra's "Could not recover jumptable" warning at the end is misleading;
 * what is there is a `pop {r0}; bx r0` interworking return (rule 35 -> void).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/actor_reset.c
 */

#include "gba_types.h"

#define STATE_FLAG 0x140000
#define PART_BIT   0x80

typedef struct Detail {
    u8 pad0[0x3e];
    u16 bits;                 /* +0x3E */
} Detail;

typedef struct Entity {
    u8 pad0[8];
    u8 kind;                  /* +0x08 */
    u8 pad9[3];
    u32 flags;                /* +0x0C */
    Detail *detail;           /* +0x10 */
} Entity;

typedef struct Sub {
    u8 pad0[4];
} Sub;

typedef struct Actor {
    u8 *base;                 /* +0x00 */
    u8 pad4[4];
    u8 tag;                   /* +0x08 */
    u8 pad9[0x27];
    Entity *entity;           /* +0x30 */
    u8 pad34[8];
    Sub *sub;                 /* +0x3C */
    Sub  body;                /* +0x40 */
    u8 pad44[0x46];
    s8 lowA;                  /* +0x8A */
    u8 pad8b[0x1d];
    s8 lowB;                  /* +0xA8 */
} Actor;

extern void GetOwnerSlot(Entity *entity);
extern void FUN_08014ffc(Sub *sub, u32 flags, u8 *a, u8 *b);
extern void FUN_08015038(Sub *sub);
extern void UpdateActorFrame(Actor *self);

void ResetActor(Actor *self)
{
    Entity *ent;
    u32 mask;
    u32 flags;

    if (self == 0) return;

    ent = self->entity;
    self->sub = &self->body;
    /* The ROM materializes the result IN A VARIABLE and tests it separately
     * (`movs r0,#0 / ... / movs r0,#1 / cmp r0,#0`); the short-circuiting
     * `&&` spelling produces a direct branch and loses six bytes. */
    if (ent != 0) {
        s32 ok = 0;
        if (ent->kind == 4) ok = 1;
        if (ok) GetOwnerSlot(ent);
    }

    mask = self->entity->flags & STATE_FLAG;
    flags = ((u32)((s32)(-mask | mask) >> 31)) & PART_BIT;
    if ((ent->flags & 0x200) != 0 || (self->entity->kind & 4) == 0) {
        flags |= ((ent->detail->bits >> 2) & 7) << 9;
    }
    FUN_08014ffc(self->sub, flags | 3, self->base, self->base + 0xc);
    FUN_08015038(self->sub);
    self->tag = 99;
    /* The ROM builds the mask as -16 with `movs #16 / negs`, not as 0xF0:
     * in the source `~15` is held in a local of WIDE type.  Written directly
     * as `&= ~15` the field is u8, so the compiler narrows the constant to
     * 0xF0 and the `negs` instruction disappears. */
    self->lowA &= ~15;
    self->lowB &= ~15;
    UpdateActorFrame(self);
}
