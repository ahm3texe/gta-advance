/* Hedefe yonelme denemesi — 0x0803F4BC-0x0803F62F
 *
 * Iki nesneyi cozup bayraklarini sinar, aralarindaki mesafeyi SEKIZGEN
 * YAKLASIMLA olcer (|dx|+|dy| - min/2 - min/4 + min/16) ve esikleri
 * gecerse FUN_0803CE64 ile bildirim yapar.
 *
 * Konum secimi: +0x08 baytinin 0x30 maskesi doluysa (+0x20)+4, degilse
 * +0x18 kullaniliyor -- iki nesne icin de ayni kalip.
 *
 * Kural 33: maskeler AYRI sonuc yereline konup yerinde `&=` yapiliyor
 * (ROM `movs r0,#48` ile maskeyi ONCE kuruyor).
 * Kural 35: `pop {r1}; bx r1` -> r0 donus degeri tasiyor, imza u32.
 *
 * ORTA BOY OLCEKLENDIRME DENEYI (372 bayt). Sonuc: 211/372.
 *
 * NE OLCEKLENDI: yapisal cozumleme. Boyut TAM DOGRU (372/372), prolog
 * birebir, ilk 10 halfword birebir, cagri dizisi birebir. Yani ROM'un ne
 * yaptigini okumak ve C'ye cevirmek 372 baytta da calisiyor.
 *
 * NE OLCEKLENMEDI: register dagitimi. +0x14'ten itibaren kaydiriyor --
 * ROM ilk sonucu r5'e, ctx->obj'i r6'ya koyuyor; bizimki r4 ve r5
 * kullaniyor. Tek register'lik bu kayma fonksiyonun geri kalanina
 * yayiliyor ve 130 halfword'u birden bozuyor.
 *
 * Denendi: yerel sayisini azaltmak (depth ve angle yerellerini dist ile
 * paylastirmak) -- cikti KISALDI, daha kotu.
 *
 * TESHIS: bu, kucuk fonksiyonlarda 88-100 baytta karsilastigim "bir fazla
 * canli deger" sinifinin ta kendisi (bkz. ClipBounds, CleanupAreaTiles,
 * UpdateFocusPoint). Fark su: 100 baytta tek bir yerde cikiyor ve
 * kovalanabiliyor; 372 baytta r8/r9'a tasan alti canli deger var ve
 * kayma her yere yayiliyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/try_engage_target.c
 */

#include "gba_types.h"

#define POS_ALT_MASK   0x30
#define BUSY_BIT       1
#define NOTIFY_BIT     (0x80 << 3)
#define NOTIFY_CLEAR   0xFBFF
#define GATE_BIT       (0x80 << 9)
#define DIST_LIMIT     0x00FFFFFF
#define DEPTH_BIAS     0xFFFFFF00
#define DEPTH_LIMIT    (0x80 << 2)
#define ANGLE_MASK     0x3FF
#define ANGLE_BIAS     0x80
#define NOTIFY_KIND    (0xA0 << 12)
#define PROBE_RANGE    0x40

typedef struct Vec2 {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
} Vec2;

typedef struct Pose {
    u8   pad00[12];
    s32  depth;                 /* +0x0C */
    u16  angle;                 /* +0x0E */
} Pose;

typedef struct Obj {
    u8    pad00[8];
    u8    kind;                 /* +0x08 */
    u8    pad09[3];
    u32   flags;                /* +0x0C */
    u8    pad10[8];
    Pose *pose;                 /* +0x18 */
    u8    pad1C[4];
    Vec2 *alt;                  /* +0x20 */
    u8    pad24[8];
    struct Obj *link;           /* +0x2C */
} Obj;

typedef struct Slot {
    u8   pad00[24];
    u16  gate;                  /* +0x18 */
} Slot;

typedef struct Ctx {
    Obj *obj;                   /* +0x00 */
    u8   pad04[40];
    u32  arg;                   /* +0x2C */
} Ctx;

typedef struct Peer {
    u8    pad00[24];
    u16   gate;                 /* +0x18 */
    u8    pad1A[4];
    u16   notify;               /* +0x1E */
    u8    pad20[12];
    Slot *slot;                 /* +0x2C */
} Peer;

extern u32   FUN_0804fc48(u32 arg, Obj *obj);
extern u32   FUN_0803c400(u32 arg);
extern u32   SelectSlotAB(u32 arg);
extern u32   GetBaseAlt(void);
extern u32   FUN_08042144(Obj *a, Obj *b, u32 range);
extern void  FUN_0803ce64(Ctx *ctx, u32 kind, u32 angle, u32 zero);

/* 0x0803F4BC */
u32 TryEngageTarget(Ctx *ctx)
{
    Obj *self;
    Obj *other;
    Peer *peer;
    Vec2 *pa;
    Vec2 *pb;
    u32 mask;
    u32 flag;
    u32 dist;
    s32 dx;
    s32 dy;
    s32 lo;
    u32 depth;
    u32 angle;

    other = (Obj *)FUN_0804fc48(ctx->arg, ctx->obj);
    self = ctx->obj;
    pa = (Vec2 *)SelectSlotAB(FUN_0803c400((u32)other));

    peer = (Peer *)self->link;
    flag = 0;
    if (peer != 0) {
        mask = BUSY_BIT;
        mask &= peer->slot->gate;
        if (mask != 0)
            goto no;
        mask = NOTIFY_BIT;
        mask &= peer->notify;
        if (mask != 0)
            flag = 1;
        peer->notify = peer->notify & NOTIFY_CLEAR;
    }

    mask = GATE_BIT;
    mask &= self->flags;
    if (mask == 0)
        goto no;
    if (GetBaseAlt() == 0)
        goto no;
    if (pa == 0)
        goto no;

    if (other == 0 || self == 0) {
        dist = 0x7FFFFFFF;
        goto limit;
    }

    mask = POS_ALT_MASK;
    mask &= other->kind;
    if (mask != 0)
        pa = other->alt + 1;
    else
        pa = (Vec2 *)other->pose;

    mask = POS_ALT_MASK;
    mask &= self->kind;
    if (mask != 0)
        pb = self->alt + 1;
    else
        pb = (Vec2 *)self->pose;

    dx = pa->x - pb->x;
    if (dx < 0)
        dx = -dx;
    dy = pa->y - pb->y;
    if (dy < 0)
        dy = -dy;
    lo = dy;
    if (lo > dx)
        lo = dx;
    dx = dx + dy - (lo >> 1) - (lo >> 2) + (lo >> 4);
    if (dx < 0)
        dx = -dx;
    dist = dx;

limit:
    if (dist > DIST_LIMIT)
        goto no;

    depth = other->pose->depth - self->pose->depth;
    depth = (depth << 6) >> 22;
    depth = depth + DEPTH_BIAS;
    if (depth <= DEPTH_LIMIT)
        goto no;

    if (flag != 0)
        goto notify;
    if (FUN_08042144(self, other, PROBE_RANGE) != 0)
        goto notify;

no:
    return 0;

notify:
    if (FUN_08042144(self, other, PROBE_RANGE) != 0) {
        angle = (s16)other->pose->angle;
        angle = (angle + ANGLE_BIAS) & ANGLE_MASK;
    } else {
        angle = other->pose->angle & ANGLE_MASK;
    }

    FUN_0803ce64(ctx, NOTIFY_KIND, angle, 0);

    if (peer != 0)
        peer->notify = NOTIFY_BIT | peer->notify;
    return 1;
}
