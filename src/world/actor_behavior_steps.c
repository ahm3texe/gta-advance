/* Aktor davranis adimlari — 0x08017E3C-0x08018B73, 12 fonksiyon
 *
 * TEK SABLONUN 12 KOPYASI.  tools/find_twins.py bandi %98.5 benzerlikle
 * isaret etti; ROM govdeleri komut komut karsilastirilinca dokuz 272
 * baytlik uyenin YALNIZCA BIR SABIT'te (FUN_080198e4'e verilen kimlik),
 * uc 312 baytlik uyenin de yalnizca "harman varken kullanilan sinir"
 * degerinde (3 ya da 4) ayrildigi gorildu.  Yontem: docs/WORKFLOW.md §10.
 *
 * IKI SABLON ARASINDAKI TEK YAPISAL FARK, 312 baytlik surumde her yolun
 * FUN_080198e4'e KENDI kimligini vermesi (125/126/127/128/123); 272
 * baytlik surumde hepsi ayni kimligi verdigi icin derleyici kuyruklari
 * birlestiriyor.
 *
 * 272 SURUMUNDE BELIRLEYICI OLAN (1 bayt farkla tikanmisti):
 *   - `!=0` yolunun IKI DALI DA kendi FUN_080198e4 cagrisini tasimali
 *     (312 surumundeki gibi), ama switch'inki TEK ORTAK cagri olmali.
 *     Ikisi de ortak yazilirsa 264 bayt cikiyor (8 bayt fazla birlestirme);
 *     ikisi de ayri yazilirsa 0x08017FFC'deki dal case 3'un kuyruguna
 *     baglaniyor, ROM'da case 1'in kuyruguna bagli (tam olarak 1 BAYT).
 *
 * OLCULEN DIGER UC AYRINTI:
 *   - Sayac `gRam020230B4++ > limit` yazilmali; ayri yerelle okuyup geri
 *     yazmak ROM'un `lsls #24 / lsrs #24` sifir genisletmesini ve
 *     isaretsiz `bls` karsilastirmasini vermiyor (limit de u32 olmali).
 *   - `SelectWordSource` ve `FUN_0803c708` ayri yerellere alinmali; tek
 *     ifadede `x & y` yazilirsa `ands` hedefi ters donuyor.
 *   - Yon hesabinda ONCE bayrak, SONRA +0x0C okunmali; tek ifadede
 *     yazilirsa +0x0C yuklemesi cagrinin ARDINA kayiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/actor_behavior_steps.c
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
extern s32  FUN_08017d78(u32 flags);
extern void RequestActorAction(Actor *self, s32 a, s32 b, s32 c);
extern void FUN_080198e4(Actor *self, s32 a, s32 b, s32 c);

/* 0x08017E3C — 312 bayt */
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
        dir = ((FUN_08017d78(flags) - base + ROUND_BIAS) >> 24) & 3;
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

/* 0x08017F74 — 272 bayt */
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
        dir = ((FUN_08017d78(flags) - base + ROUND_BIAS) >> 24) & 3;
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

/* 0x08018084 — 272 bayt */
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
        dir = ((FUN_08017d78(flags) - base + ROUND_BIAS) >> 24) & 3;
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

/* 0x08018194 — 272 bayt */
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
        dir = ((FUN_08017d78(flags) - base + ROUND_BIAS) >> 24) & 3;
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

/* 0x080182A4 — 272 bayt */
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
        dir = ((FUN_08017d78(flags) - base + ROUND_BIAS) >> 24) & 3;
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

/* 0x080183B4 — 272 bayt */
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
        dir = ((FUN_08017d78(flags) - base + ROUND_BIAS) >> 24) & 3;
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

/* 0x080184C4 — 272 bayt */
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
        dir = ((FUN_08017d78(flags) - base + ROUND_BIAS) >> 24) & 3;
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

/* 0x080185D4 — 272 bayt */
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
        dir = ((FUN_08017d78(flags) - base + ROUND_BIAS) >> 24) & 3;
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

/* 0x080186E4 — 312 bayt */
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
        dir = ((FUN_08017d78(flags) - base + ROUND_BIAS) >> 24) & 3;
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

/* 0x0801881C — 312 bayt */
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
        dir = ((FUN_08017d78(flags) - base + ROUND_BIAS) >> 24) & 3;
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

/* 0x08018954 — 272 bayt */
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
        dir = ((FUN_08017d78(flags) - base + ROUND_BIAS) >> 24) & 3;
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

/* 0x08018A64 — 272 bayt */
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
        dir = ((FUN_08017d78(flags) - base + ROUND_BIAS) >> 24) & 3;
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
