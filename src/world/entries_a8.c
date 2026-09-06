/* TASLAK -- gRam020242B0 tablosuna yeni giris kurma -- 0x08028894-0x080289AB */

#include "gba_types.h"

#define ENTRY_COUNT   1
#define ROOT_SLOT     34
#define SUB_ARG       3
#define AGE_LIMIT     0x004FFFFF
#define SPAWN_BITS    0x01000000
#define FLAG_CLEAR    15

typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

typedef struct RomNode {
    u32             pad00;
    struct RomNode **slots;     /* +0x04 */
    u8              pad08[12];
    u32             unk14;      /* +0x14 */
} RomNode;

extern RomNode gRom08BD3448;

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;                      /* +0x08 */
} Vec3;

extern Vec3 gClipBounds;
extern u32  gRam03000078;

typedef struct SourceState {
    Triple t;                   /* +0x00 */
    u32    unk0c;               /* +0x0C */
} SourceState;

typedef struct Holder {
    u8           pad00[24];
    SourceState *unk18;         /* +0x18 */
} Holder;

typedef struct Source {
    u8       pad00[0x24];
    RomNode *node;              /* +0x24 */
    u8       pad28[0x62];
    s8       flags;             /* +0x8A */
    u8       pad8b[0x26];
    u8       slot;              /* +0xB1 */
} Source;

typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     pad01;
    u16    unk02;               /* +0x02 */
    u8     sub[38];             /* +0x04 */
    u8     pad2a[34];
    Triple payload;             /* +0x4C */
    Triple payload2;            /* +0x58 */
    u8     kind;                /* +0x64 */
    u8     pad65[3];
    u32    unk68;               /* +0x68 */
    u8     pad6c[20];
    u32    unk80;               /* +0x80 */
    u8     pad84[12];
    u32    phase;               /* +0x90 */
} Entry;

extern Entry gRam020242B0[];

extern void FUN_08014ffc(u8 *dest, u32 count, Triple *src, u32 *value);
extern void FUN_08013cfc(u8 *dest, RomNode *desc, u32 arg);
extern void FUN_08014ee4(u8 *dest, u32 value);
extern void FUN_08015038(u8 *dest);
extern void FUN_08025340(Entry *e, s32 turn, s32 speed, void *off);

/* 0x08028894 */
void FUN_08028894(Source *h, Holder *src, u32 phase, u32 kind)
{
    Entry *e;
    RomNode *node;
    RomNode *desc;
    u32 arg;
    s32 age;
    u32 i;

    arg = gRam03000078;

    for (i = 0; i < ENTRY_COUNT; i++) {
        if (gRam020242B0[i].active == 0)
            break;
    }

    if (i == ENTRY_COUNT)
        return;

    e = &gRam020242B0[i];
    h->slot = i;
    e->unk80 = 0;
    e->phase = phase;
    e->kind = kind;

    node = gRom08BD3448.slots[ROOT_SLOT];
    h->node = node;
    desc = node->slots[phase]->slots[0];

    age = e->payload.c;
    if (age != 0) {
        if (gClipBounds.z - age <= AGE_LIMIT)
            arg = 0;
    }

    e->payload = src->unk18->t;
    e->payload2 = src->unk18->t;
    e->unk68 = src->unk18->unk0c;

    FUN_08014ffc(e->sub, SUB_ARG, &e->payload, &e->unk68);
    FUN_08013cfc(e->sub, desc, arg);
    FUN_08014ee4(e->sub, desc->unk14);
    FUN_08015038(e->sub);

    FUN_08025340(e, 0, kind, 0);
    FUN_08025340(e, SPAWN_BITS, 2, 0);

    e->active = 1;
    h->flags &= ~FLAG_CLEAR;
    e->unk02 = 0;
}
