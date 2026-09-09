/* Actor behavior steps — 0x08017E3C-0x08018B73, 12 functions
 *
 * 12 COPIES OF A SINGLE TEMPLATE.  tools/find_twins.py flagged the band at
 * 98.5% similarity; comparing the ROM bodies instruction by instruction showed
 * that the nine 272-byte members differ in only ONE CONSTANT (the id passed to
 * FUN_080198e4), and the three 312-byte members only in the "limit used when a
 * blend is present" value (3 or 4).  Method: docs/WORKFLOW.md section 10.
 *
 * THE ONLY STRUCTURAL DIFFERENCE BETWEEN THE TWO TEMPLATES is that in the
 * 312-byte version each path passes its OWN id to FUN_080198e4
 * (125/126/127/128/123); in the 272-byte version they all pass the same id, so
 * the compiler merges the tails.
 *
 * WHAT WAS DECISIVE IN THE 272 VERSION (it had been stuck 1 byte off):
 *   - BOTH BRANCHES of the `!=0` path must carry their own FUN_080198e4 call
 *     (as in the 312 version), but the switch's must be a SINGLE SHARED call.
 *     Written as shared in both places it comes out at 264 bytes (8 bytes of
 *     over-merging); written separately in both, the branch at 0x08017FFC
 *     links to case 3's tail, whereas in the ROM it links to case 1's tail
 *     (exactly 1 BYTE).
 *
 * THREE OTHER MEASURED DETAILS:
 *   - The counter must be written `gRam020230B4++ > limit`; reading into a
 *     separate local and writing back does not give the ROM's
 *     `lsls #24 / lsrs #24` zero-extension or its unsigned `bls` comparison
 *     (the limit must be u32 as well).
 *   - `SelectWordSource` and `FUN_0803c708` must be taken into separate
 *     locals; written as `x & y` in a single expression, the `ands`
 *     destination is reversed.
 *   - In the direction computation the flag must be read FIRST and +0x0C
 *     SECOND; written as a single expression the +0x0C load moves AFTER the
 *     call.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/actor_behavior_steps.c
 */

#include "gba_types.h"

#define BLEND_MASK   0x6300
#define ROUND_BIAS   0x800000
#define RETRY_ARG    264

#define ACT_STOP     107
#define ACT_A        109
#define ACT_B        110
#define ACT_C        111
#define ACT_D        112

typedef struct Owner {
    u8    pad00[12];
    s32   unk0C;                /* +0x0C */
    u8    pad10[8];
    void *unk18;                /* +0x18 */
    u8    pad1c[16];
    u16   flags;                /* +0x2C */
} Owner;

typedef struct Entity {
    u8  pad00[24];
    u8 *unk18;                  /* +0x18 */
} Entity;

typedef struct Actor {
    Owner  *owner;              /* +0x00 */
    u8      pad04[44];
    Entity *entity;             /* +0x30 */
} Actor;

extern u8 gRam020230B4;

extern u32  GetOwnerSlot(Entity *entity);
extern u32  SelectWordSource(u32 slot);
extern u32  FUN_0803c708(u32 slot);
extern s32  FUN_08019320(Entity *entity);
extern void FUN_080350a8(Entity *entity, s32 arg);
extern s32  FlagsToAngle(u32 flags);
extern void RequestActorAction(Actor *self, s32 a, s32 b, s32 c);
extern void FUN_080198e4(Actor *self, s32 a, s32 b, s32 c);

/* 0x08017E3C — 312 bytes */
void StepActorBehaviorB3(Actor *self)
{
    Owner  *owner;
    Entity *entity;
    u32     slot;
    u32     limit;
    s32     dir;
    u32     mask;
    u32     bits;
    s32     base;
    u32     flags;

    limit = 5;
    slot = GetOwnerSlot(self->entity);
    owner = self->owner;
    if (owner->unk18 != 0) {
        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask))
            limit = 3;

        entity = self->entity;
        if (*(entity->unk18 + 0x30) != 2 && FUN_08019320(entity) == 0) {
            if (gRam020230B4++ > limit) {
                gRam020230B4 = 0;
                FUN_080350a8(self->entity, RETRY_ARG);
            }
        }

        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask)) {
            RequestActorAction(self, ACT_A, 4, 2);
            FUN_080198e4(self, 125, 2, 0);
        } else {
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 126, 2, 0);
        }
    } else if (owner->flags & BLEND_MASK) {
        flags = owner->flags;
        base = owner->unk0C;
        dir = ((FlagsToAngle(flags) - base + ROUND_BIAS) >> 24) & 3;
        switch (dir) {
        case 0:
        case 2:
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 126, 2, 0);
            break;
        case 1:
            RequestActorAction(self, ACT_D, 4, 2);
            FUN_080198e4(self, 128, 2, 0);
            break;
        case 3:
            RequestActorAction(self, ACT_C, 4, 2);
            FUN_080198e4(self, 127, 2, 0);
            break;
        }
    } else {
        RequestActorAction(self, ACT_STOP, 2, 2);
        FUN_080198e4(self, 123, 2, 8);
    }
}

/* 0x08017F74 — 272 bytes */
void StepActorBehavior136(Actor *self)
{
    Owner  *owner;
    Entity *entity;
    u32     slot;
    u32     limit;
    s32     dir;
    u32     mask;
    u32     bits;
    s32     base;
    u32     flags;

    limit = 5;
    slot = GetOwnerSlot(self->entity);
    owner = self->owner;
    if (owner->unk18 != 0) {
        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask))
            limit = 4;

        entity = self->entity;
        if (*(entity->unk18 + 0x30) != 2 && FUN_08019320(entity) == 0) {
            if (gRam020230B4++ > limit) {
                gRam020230B4 = 0;
                FUN_080350a8(self->entity, RETRY_ARG);
            }
        }

        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask)) {
            RequestActorAction(self, ACT_A, 4, 2);
            FUN_080198e4(self, 136, 2, 0);
        } else {
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 136, 2, 0);
        }
    } else if (owner->flags & BLEND_MASK) {
        flags = owner->flags;
        base = owner->unk0C;
        dir = ((FlagsToAngle(flags) - base + ROUND_BIAS) >> 24) & 3;
        switch (dir) {
        case 0:
        case 2:
            RequestActorAction(self, ACT_B, 4, 2);
            break;
        case 1:
            RequestActorAction(self, ACT_D, 4, 2);
            break;
        case 3:
            RequestActorAction(self, ACT_C, 4, 2);
            break;
        }
        FUN_080198e4(self, 136, 2, 0);
    } else {
        RequestActorAction(self, ACT_STOP, 2, 2);
        FUN_080198e4(self, 136, 2, 8);
    }
}

/* 0x08018084 — 272 bytes */
void StepActorBehavior155(Actor *self)
{
    Owner  *owner;
    Entity *entity;
    u32     slot;
    u32     limit;
    s32     dir;
    u32     mask;
    u32     bits;
    s32     base;
    u32     flags;

    limit = 5;
    slot = GetOwnerSlot(self->entity);
    owner = self->owner;
    if (owner->unk18 != 0) {
        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask))
            limit = 4;

        entity = self->entity;
        if (*(entity->unk18 + 0x30) != 2 && FUN_08019320(entity) == 0) {
            if (gRam020230B4++ > limit) {
                gRam020230B4 = 0;
                FUN_080350a8(self->entity, RETRY_ARG);
            }
        }

        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask)) {
            RequestActorAction(self, ACT_A, 4, 2);
            FUN_080198e4(self, 155, 2, 0);
        } else {
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 155, 2, 0);
        }
    } else if (owner->flags & BLEND_MASK) {
        flags = owner->flags;
        base = owner->unk0C;
        dir = ((FlagsToAngle(flags) - base + ROUND_BIAS) >> 24) & 3;
        switch (dir) {
        case 0:
        case 2:
            RequestActorAction(self, ACT_B, 4, 2);
            break;
        case 1:
            RequestActorAction(self, ACT_D, 4, 2);
            break;
        case 3:
            RequestActorAction(self, ACT_C, 4, 2);
            break;
        }
        FUN_080198e4(self, 155, 2, 0);
    } else {
        RequestActorAction(self, ACT_STOP, 2, 2);
        FUN_080198e4(self, 155, 2, 8);
    }
}

/* 0x08018194 — 272 bytes */
void StepActorBehavior139(Actor *self)
{
    Owner  *owner;
    Entity *entity;
    u32     slot;
    u32     limit;
    s32     dir;
    u32     mask;
    u32     bits;
    s32     base;
    u32     flags;

    limit = 5;
    slot = GetOwnerSlot(self->entity);
    owner = self->owner;
    if (owner->unk18 != 0) {
        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask))
            limit = 4;

        entity = self->entity;
        if (*(entity->unk18 + 0x30) != 2 && FUN_08019320(entity) == 0) {
            if (gRam020230B4++ > limit) {
                gRam020230B4 = 0;
                FUN_080350a8(self->entity, RETRY_ARG);
            }
        }

        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask)) {
            RequestActorAction(self, ACT_A, 4, 2);
            FUN_080198e4(self, 139, 2, 0);
        } else {
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 139, 2, 0);
        }
    } else if (owner->flags & BLEND_MASK) {
        flags = owner->flags;
        base = owner->unk0C;
        dir = ((FlagsToAngle(flags) - base + ROUND_BIAS) >> 24) & 3;
        switch (dir) {
        case 0:
        case 2:
            RequestActorAction(self, ACT_B, 4, 2);
            break;
        case 1:
            RequestActorAction(self, ACT_D, 4, 2);
            break;
        case 3:
            RequestActorAction(self, ACT_C, 4, 2);
            break;
        }
        FUN_080198e4(self, 139, 2, 0);
    } else {
        RequestActorAction(self, ACT_STOP, 2, 2);
        FUN_080198e4(self, 139, 2, 8);
    }
}

/* 0x080182A4 — 272 bytes */
void StepActorBehavior142(Actor *self)
{
    Owner  *owner;
    Entity *entity;
    u32     slot;
    u32     limit;
    s32     dir;
    u32     mask;
    u32     bits;
    s32     base;
    u32     flags;

    limit = 5;
    slot = GetOwnerSlot(self->entity);
    owner = self->owner;
    if (owner->unk18 != 0) {
        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask))
            limit = 4;

        entity = self->entity;
        if (*(entity->unk18 + 0x30) != 2 && FUN_08019320(entity) == 0) {
            if (gRam020230B4++ > limit) {
                gRam020230B4 = 0;
                FUN_080350a8(self->entity, RETRY_ARG);
            }
        }

        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask)) {
            RequestActorAction(self, ACT_A, 4, 2);
            FUN_080198e4(self, 142, 2, 0);
        } else {
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 142, 2, 0);
        }
    } else if (owner->flags & BLEND_MASK) {
        flags = owner->flags;
        base = owner->unk0C;
        dir = ((FlagsToAngle(flags) - base + ROUND_BIAS) >> 24) & 3;
        switch (dir) {
        case 0:
        case 2:
            RequestActorAction(self, ACT_B, 4, 2);
            break;
        case 1:
            RequestActorAction(self, ACT_D, 4, 2);
            break;
        case 3:
            RequestActorAction(self, ACT_C, 4, 2);
            break;
        }
        FUN_080198e4(self, 142, 2, 0);
    } else {
        RequestActorAction(self, ACT_STOP, 2, 2);
        FUN_080198e4(self, 142, 2, 8);
    }
}

/* 0x080183B4 — 272 bytes */
void StepActorBehavior149(Actor *self)
{
    Owner  *owner;
    Entity *entity;
    u32     slot;
    u32     limit;
    s32     dir;
    u32     mask;
    u32     bits;
    s32     base;
    u32     flags;

    limit = 5;
    slot = GetOwnerSlot(self->entity);
    owner = self->owner;
    if (owner->unk18 != 0) {
        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask))
            limit = 4;

        entity = self->entity;
        if (*(entity->unk18 + 0x30) != 2 && FUN_08019320(entity) == 0) {
            if (gRam020230B4++ > limit) {
                gRam020230B4 = 0;
                FUN_080350a8(self->entity, RETRY_ARG);
            }
        }

        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask)) {
            RequestActorAction(self, ACT_A, 4, 2);
            FUN_080198e4(self, 149, 2, 0);
        } else {
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 149, 2, 0);
        }
    } else if (owner->flags & BLEND_MASK) {
        flags = owner->flags;
        base = owner->unk0C;
        dir = ((FlagsToAngle(flags) - base + ROUND_BIAS) >> 24) & 3;
        switch (dir) {
        case 0:
        case 2:
            RequestActorAction(self, ACT_B, 4, 2);
            break;
        case 1:
            RequestActorAction(self, ACT_D, 4, 2);
            break;
        case 3:
            RequestActorAction(self, ACT_C, 4, 2);
            break;
        }
        FUN_080198e4(self, 149, 2, 0);
    } else {
        RequestActorAction(self, ACT_STOP, 2, 2);
        FUN_080198e4(self, 149, 2, 8);
    }
}

/* 0x080184C4 — 272 bytes */
void StepActorBehavior152(Actor *self)
{
    Owner  *owner;
    Entity *entity;
    u32     slot;
    u32     limit;
    s32     dir;
    u32     mask;
    u32     bits;
    s32     base;
    u32     flags;

    limit = 5;
    slot = GetOwnerSlot(self->entity);
    owner = self->owner;
    if (owner->unk18 != 0) {
        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask))
            limit = 4;

        entity = self->entity;
        if (*(entity->unk18 + 0x30) != 2 && FUN_08019320(entity) == 0) {
            if (gRam020230B4++ > limit) {
                gRam020230B4 = 0;
                FUN_080350a8(self->entity, RETRY_ARG);
            }
        }

        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask)) {
            RequestActorAction(self, ACT_A, 4, 2);
            FUN_080198e4(self, 152, 2, 0);
        } else {
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 152, 2, 0);
        }
    } else if (owner->flags & BLEND_MASK) {
        flags = owner->flags;
        base = owner->unk0C;
        dir = ((FlagsToAngle(flags) - base + ROUND_BIAS) >> 24) & 3;
        switch (dir) {
        case 0:
        case 2:
            RequestActorAction(self, ACT_B, 4, 2);
            break;
        case 1:
            RequestActorAction(self, ACT_D, 4, 2);
            break;
        case 3:
            RequestActorAction(self, ACT_C, 4, 2);
            break;
        }
        FUN_080198e4(self, 152, 2, 0);
    } else {
        RequestActorAction(self, ACT_STOP, 2, 2);
        FUN_080198e4(self, 152, 2, 8);
    }
}

/* 0x080185D4 — 272 bytes */
void StepActorBehavior146(Actor *self)
{
    Owner  *owner;
    Entity *entity;
    u32     slot;
    u32     limit;
    s32     dir;
    u32     mask;
    u32     bits;
    s32     base;
    u32     flags;

    limit = 5;
    slot = GetOwnerSlot(self->entity);
    owner = self->owner;
    if (owner->unk18 != 0) {
        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask))
            limit = 4;

        entity = self->entity;
        if (*(entity->unk18 + 0x30) != 2 && FUN_08019320(entity) == 0) {
            if (gRam020230B4++ > limit) {
                gRam020230B4 = 0;
                FUN_080350a8(self->entity, RETRY_ARG);
            }
        }

        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask)) {
            RequestActorAction(self, ACT_A, 4, 2);
            FUN_080198e4(self, 146, 2, 0);
        } else {
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 146, 2, 0);
        }
    } else if (owner->flags & BLEND_MASK) {
        flags = owner->flags;
        base = owner->unk0C;
        dir = ((FlagsToAngle(flags) - base + ROUND_BIAS) >> 24) & 3;
        switch (dir) {
        case 0:
        case 2:
            RequestActorAction(self, ACT_B, 4, 2);
            break;
        case 1:
            RequestActorAction(self, ACT_D, 4, 2);
            break;
        case 3:
            RequestActorAction(self, ACT_C, 4, 2);
            break;
        }
        FUN_080198e4(self, 146, 2, 0);
    } else {
        RequestActorAction(self, ACT_STOP, 2, 2);
        FUN_080198e4(self, 146, 2, 8);
    }
}

/* 0x080186E4 — 312 bytes */
void StepActorBehaviorB4a(Actor *self)
{
    Owner  *owner;
    Entity *entity;
    u32     slot;
    u32     limit;
    s32     dir;
    u32     mask;
    u32     bits;
    s32     base;
    u32     flags;

    limit = 5;
    slot = GetOwnerSlot(self->entity);
    owner = self->owner;
    if (owner->unk18 != 0) {
        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask))
            limit = 4;

        entity = self->entity;
        if (*(entity->unk18 + 0x30) != 2 && FUN_08019320(entity) == 0) {
            if (gRam020230B4++ > limit) {
                gRam020230B4 = 0;
                FUN_080350a8(self->entity, RETRY_ARG);
            }
        }

        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask)) {
            RequestActorAction(self, ACT_A, 4, 2);
            FUN_080198e4(self, 125, 2, 0);
        } else {
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 126, 2, 0);
        }
    } else if (owner->flags & BLEND_MASK) {
        flags = owner->flags;
        base = owner->unk0C;
        dir = ((FlagsToAngle(flags) - base + ROUND_BIAS) >> 24) & 3;
        switch (dir) {
        case 0:
        case 2:
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 126, 2, 0);
            break;
        case 1:
            RequestActorAction(self, ACT_D, 4, 2);
            FUN_080198e4(self, 128, 2, 0);
            break;
        case 3:
            RequestActorAction(self, ACT_C, 4, 2);
            FUN_080198e4(self, 127, 2, 0);
            break;
        }
    } else {
        RequestActorAction(self, ACT_STOP, 2, 2);
        FUN_080198e4(self, 123, 2, 8);
    }
}

/* 0x0801881C — 312 bytes */
void StepActorBehaviorB4b(Actor *self)
{
    Owner  *owner;
    Entity *entity;
    u32     slot;
    u32     limit;
    s32     dir;
    u32     mask;
    u32     bits;
    s32     base;
    u32     flags;

    limit = 5;
    slot = GetOwnerSlot(self->entity);
    owner = self->owner;
    if (owner->unk18 != 0) {
        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask))
            limit = 4;

        entity = self->entity;
        if (*(entity->unk18 + 0x30) != 2 && FUN_08019320(entity) == 0) {
            if (gRam020230B4++ > limit) {
                gRam020230B4 = 0;
                FUN_080350a8(self->entity, RETRY_ARG);
            }
        }

        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask)) {
            RequestActorAction(self, ACT_A, 4, 2);
            FUN_080198e4(self, 125, 2, 0);
        } else {
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 126, 2, 0);
        }
    } else if (owner->flags & BLEND_MASK) {
        flags = owner->flags;
        base = owner->unk0C;
        dir = ((FlagsToAngle(flags) - base + ROUND_BIAS) >> 24) & 3;
        switch (dir) {
        case 0:
        case 2:
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 126, 2, 0);
            break;
        case 1:
            RequestActorAction(self, ACT_D, 4, 2);
            FUN_080198e4(self, 128, 2, 0);
            break;
        case 3:
            RequestActorAction(self, ACT_C, 4, 2);
            FUN_080198e4(self, 127, 2, 0);
            break;
        }
    } else {
        RequestActorAction(self, ACT_STOP, 2, 2);
        FUN_080198e4(self, 123, 2, 8);
    }
}

/* 0x08018954 — 272 bytes */
void StepActorBehavior130(Actor *self)
{
    Owner  *owner;
    Entity *entity;
    u32     slot;
    u32     limit;
    s32     dir;
    u32     mask;
    u32     bits;
    s32     base;
    u32     flags;

    limit = 5;
    slot = GetOwnerSlot(self->entity);
    owner = self->owner;
    if (owner->unk18 != 0) {
        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask))
            limit = 4;

        entity = self->entity;
        if (*(entity->unk18 + 0x30) != 2 && FUN_08019320(entity) == 0) {
            if (gRam020230B4++ > limit) {
                gRam020230B4 = 0;
                FUN_080350a8(self->entity, RETRY_ARG);
            }
        }

        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask)) {
            RequestActorAction(self, ACT_A, 4, 2);
            FUN_080198e4(self, 130, 2, 0);
        } else {
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 130, 2, 0);
        }
    } else if (owner->flags & BLEND_MASK) {
        flags = owner->flags;
        base = owner->unk0C;
        dir = ((FlagsToAngle(flags) - base + ROUND_BIAS) >> 24) & 3;
        switch (dir) {
        case 0:
        case 2:
            RequestActorAction(self, ACT_B, 4, 2);
            break;
        case 1:
            RequestActorAction(self, ACT_D, 4, 2);
            break;
        case 3:
            RequestActorAction(self, ACT_C, 4, 2);
            break;
        }
        FUN_080198e4(self, 130, 2, 0);
    } else {
        RequestActorAction(self, ACT_STOP, 2, 2);
        FUN_080198e4(self, 130, 2, 8);
    }
}

/* 0x08018A64 — 272 bytes */
void StepActorBehavior133(Actor *self)
{
    Owner  *owner;
    Entity *entity;
    u32     slot;
    u32     limit;
    s32     dir;
    u32     mask;
    u32     bits;
    s32     base;
    u32     flags;

    limit = 5;
    slot = GetOwnerSlot(self->entity);
    owner = self->owner;
    if (owner->unk18 != 0) {
        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask))
            limit = 4;

        entity = self->entity;
        if (*(entity->unk18 + 0x30) != 2 && FUN_08019320(entity) == 0) {
            if (gRam020230B4++ > limit) {
                gRam020230B4 = 0;
                FUN_080350a8(self->entity, RETRY_ARG);
            }
        }

        mask = SelectWordSource(slot);
        bits = FUN_0803c708(slot);
        if ((u16)(bits & mask)) {
            RequestActorAction(self, ACT_A, 4, 2);
            FUN_080198e4(self, 133, 2, 0);
        } else {
            RequestActorAction(self, ACT_B, 4, 2);
            FUN_080198e4(self, 133, 2, 0);
        }
    } else if (owner->flags & BLEND_MASK) {
        flags = owner->flags;
        base = owner->unk0C;
        dir = ((FlagsToAngle(flags) - base + ROUND_BIAS) >> 24) & 3;
        switch (dir) {
        case 0:
        case 2:
            RequestActorAction(self, ACT_B, 4, 2);
            break;
        case 1:
            RequestActorAction(self, ACT_D, 4, 2);
            break;
        case 3:
            RequestActorAction(self, ACT_C, 4, 2);
            break;
        }
        FUN_080198e4(self, 133, 2, 0);
    } else {
        RequestActorAction(self, ACT_STOP, 2, 2);
        FUN_080198e4(self, 133, 2, 8);
    }
}
