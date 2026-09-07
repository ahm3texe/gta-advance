/* Varlik denetimi — 0x08019450-0x0801952B
 *
 * Alti kucuk fonksiyon. Varligin +0x30'daki alt nesnesi bayraklari ve
 * turu tutuyor; +0x34 bir geri cagri isaretcisi (dolayli cagri
 * `_call_via_r1` veneer'inden geciyor).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/actor_control.c
 */

#include "gba_types.h"

#define SUB_FLAG_MARKED   0x04
#define SUB_FLAG_NOTIFY   0x01
#define NOTIFY_ARG        0x10000
#define STATE_ACTIVE      2

typedef struct Actor Actor;

typedef struct ActorSub {
    u8  pad00[8];
    u8  flags;                  /* +0x08 */
    u8  kind;                   /* +0x09 */
    u8  pad0A[2];
    u32 options;                /* +0x0C */
} ActorSub;

struct Actor {
    u8        pad00[10];
    u8        state;            /* +0x0A */
    u8        pad0B[0x1D];
    u32       unk28;            /* +0x28 */
    u32       unk2C;            /* +0x2C */
    ActorSub *sub;              /* +0x30 */
    void    (*notify)(Actor *); /* +0x34 */
    u8        pad38[4];
    u32      *unk3C;            /* +0x3C */
    u8        pad40[0x50];
    u32       unk90;            /* +0x90 */
};

extern void FUN_08016990(Actor *actor, u32 arg);
extern void FUN_08013abc(u32 *value);
extern u32  RandomBelowCount(u32 id);
extern u32  GetGlyphCell(u32 id, u32 zero, u32 extra);
extern void FUN_0803bfe4(ActorSub *sub);

/* 0x08019450 */
u32 IsActorMarked(Actor *actor)
{
    if ((actor->sub->flags & SUB_FLAG_MARKED) != 0)
        return 1;

    return 0;
}

/* 0x08019464 */
void NotifyActor(Actor *actor)
{
    if ((actor->sub->options & SUB_FLAG_NOTIFY) != 0) {
        if (actor->notify != 0)
            actor->notify(actor);
        else
            FUN_08016990(actor, NOTIFY_ARG);
    }
}

/* 0x08019490 */
void ReleaseActorRefs(Actor *actor)
{
    if (actor->unk3C != 0) {
        FUN_08013abc(actor->unk3C);
        actor->unk3C = 0;
    }

    if (actor->sub != 0)
        actor->sub = 0;
}

/* 0x080194B4 */
void ResolveActorExtra(Actor *actor, int id)
{
    u32 key;
    u8  kind;

    kind = actor->sub->kind;
    if (kind == 57 || kind == 25 || kind == 21
        || kind == 47 || kind == 56 || kind == 35)
        return;

    key = (u16)id;
    actor->unk90 = GetGlyphCell(key, 0, RandomBelowCount(key));
}

/* 0x080194F4 */
void ActivateActor(Actor *actor, u32 a, u32 b)
{
    FUN_0803bfe4(actor->sub);
    actor->unk28 = a;
    actor->unk2C = b;
    actor->state = STATE_ACTIVE;
}

/* 0x08019510 */
u32 IsActorBusy(Actor *actor)
{
    u32 busy;
    u8  state;

    busy = 0;
    state = actor->state;
    if (state == 2 || state == 5 || state == 3 || state == 4)
        busy = 1;

    return busy;
}
