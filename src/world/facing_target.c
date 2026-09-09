/* Returning the target inside the facing cone — 0x08017500-0x08017627
 *
 * TWO FUNCTIONS, ONE BODY.  tools/find_twins.py reported them 100% similar;
 * comparing the ROM bodies instruction by instruction, ONLY the branch
 * targets differ, so the same source was compiled twice.
 * (docs/WORKFLOW.md section 10)
 *
 * Behaviour: the record at the context's +0x14 carries a kind byte at +0x114.
 * For kind 8 the word at +0x100 is returned directly.  For kind 1 the entity
 * there is taken; if FUN_08019320 returns non-zero the target is discarded.
 * Then the positions of the two objects -- from +0x20 plus 4 if bit 0x30 is
 * set in the +0x08 flags, otherwise from +0x18 -- are subtracted and the
 * difference is turned into an ANGLE by FUN_0800c180 (the result is masked
 * with 0x3FF, i.e. a full turn of 1024 units).  The angle is subtracted from
 * the signed facing angle at +0x0E of the record at the context's +0x18; if
 * the difference exceeds half a turn it is measured the other way round.  If
 * the remaining difference does not exceed 255 the entity is returned,
 * otherwise 0.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/facing_target.c
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
    Pos *posAlt;                    /* +0x20, used with a +4 offset */
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

/* 0x08017594 — an exact second copy of GetFacingTarget in the ROM. */
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
