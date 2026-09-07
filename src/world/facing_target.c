/* Bakis konisindeki hedefi verme — 0x08017500-0x08017627
 *
 * IKI FONKSIYON, AYNI GOVDE.  tools/find_twins.py ikisini %100 benzer
 * gosterdi; ROM govdeleri komut komut karsilastirilinca YALNIZCA dal
 * hedefleri farkli cikti, yani ayni kaynak iki kez derlenmis.
 * (docs/WORKFLOW.md §10)
 *
 * Davranis: baglamin +0x14'undeki kayit +0x114'te bir tur bayti tasiyor.
 * Tur 8 ise +0x100'daki soz dogrudan doner.  Tur 1 ise oradaki varlik
 * alinir; FUN_08019320 sifir donmezse hedef yok sayilir.  Sonra iki
 * nesnenin konumu -- +0x08'deki bayrakta 0x30 kuruluysa +0x20'den +4,
 * degilse +0x18'den -- farki alinip FUN_0800c180 ile ACIYA cevriliyor
 * (sonuc 0x3FF ile maskeleniyor, yani 1024 birimlik tam tur).  Aci,
 * baglamin +0x18'indeki kaydin +0x0E'sindeki isaretli bakis acisindan
 * cikariliyor; fark yarim turu asarsa ters yonden olculuyor.  Kalan fark
 * 255'i asmiyorsa varlik doner, asarsa 0.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/facing_target.c
 */

#include "gba_types.h"

#define REC_KIND_OFF   276          /* 138 << 1 */
#define REC_PTR_OFF    256          /* 128 << 1 */
#define KIND_DIRECT    8
#define KIND_ENTITY    1
#define POS_FLAG       0x30
#define ANGLE_MASK     0x3FF
#define HALF_TURN      512          /* 128 << 2 */
#define CONE_LIMIT     255

typedef struct Pos {
    s32 x;                          /* +0x00 */
    s32 y;                          /* +0x04 */
} Pos;

typedef struct Node {
    u8   pad00[8];
    u8   flags;                     /* +0x08 */
    u8   pad09[15];
    Pos *pos;                       /* +0x18 */
    u8   pad1c[4];
    Pos *posAlt;                    /* +0x20, +4 kaydirilarak kullaniliyor */
} Node;

typedef struct Heading {
    u8  pad00[14];
    s16 angle;                      /* +0x0E */
} Heading;

typedef struct Ctx {
    u8       pad00[8];
    u8       flags;                 /* +0x08 */
    u8       pad09[11];
    u8      *record;                /* +0x14 */
    u8       pad18[0];
} Ctx;

extern s32  FUN_08019320(Node *node);
extern s32  FUN_0800c180(s32 dx, s32 dy);

/* 0x08017500 */
Node *GetFacingTarget(Node *ctx)
{
    u8   *record;
    Node *target;
    Pos  *self;
    Pos  *other;
    s32   angle;
    s32   delta;

    record = ((Ctx *)ctx)->record;
    if (record[REC_KIND_OFF] == KIND_DIRECT)
        return *(Node **)(record + REC_PTR_OFF);
    if (record[REC_KIND_OFF] != KIND_ENTITY)
        return 0;

    target = *(Node **)(record + REC_PTR_OFF);
    if (FUN_08019320(target) != 0)
        return 0;

    if (POS_FLAG & ctx->flags)
        self = (Pos *)((u8 *)ctx->posAlt + 4);
    else
        self = ctx->pos;

    if (POS_FLAG & target->flags)
        other = (Pos *)((u8 *)target->posAlt + 4);
    else
        other = target->pos;

    angle = FUN_0800c180(other->x - self->x, other->y - self->y) & ANGLE_MASK;
    delta = (angle - ((Heading *)ctx->pos)->angle) & ANGLE_MASK;
    if (delta > HALF_TURN)
        delta = ANGLE_MASK - delta;
    if (delta <= CONE_LIMIT)
        return target;
    return 0;
}

/* 0x08017594 — ROM'da GetFacingTarget'in birebir ikinci kopyasi. */
Node *GetFacingTargetDup(Node *ctx)
{
    u8   *record;
    Node *target;
    Pos  *self;
    Pos  *other;
    s32   angle;
    s32   delta;

    record = ((Ctx *)ctx)->record;
    if (record[REC_KIND_OFF] == KIND_DIRECT)
        return *(Node **)(record + REC_PTR_OFF);
    if (record[REC_KIND_OFF] != KIND_ENTITY)
        return 0;

    target = *(Node **)(record + REC_PTR_OFF);
    if (FUN_08019320(target) != 0)
        return 0;

    if (POS_FLAG & ctx->flags)
        self = (Pos *)((u8 *)ctx->posAlt + 4);
    else
        self = ctx->pos;

    if (POS_FLAG & target->flags)
        other = (Pos *)((u8 *)target->posAlt + 4);
    else
        other = target->pos;

    angle = FUN_0800c180(other->x - self->x, other->y - self->y) & ANGLE_MASK;
    delta = (angle - ((Heading *)ctx->pos)->angle) & ANGLE_MASK;
    if (delta > HALF_TURN)
        delta = ANGLE_MASK - delta;
    if (delta <= CONE_LIMIT)
        return target;
    return 0;
}
