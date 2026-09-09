/* Computes the actor's facing bytes and the 16.16 position offset from state.
 *
 * 0x080260A8, 1518 bytes.  Branches on the STATE value at the actor's +0x90.
 * Every branch has the same skeleton:
 *
 *   1. An ANGLE is read from the entity, masked with 0x03FFFFFF (26 bits) and
 *      written to the actor's +0x68 field.  The angle turns into a 10-bit
 *      index via >> 16; FUN_08029088 takes it like a conversion helper.
 *   2. A midpoint is computed from the record for the two axes:
 *      (b6 + b4) - (b18 - 2), then halved with `<< 23 >> 24` and sign
 *      extended from 9 bits.
 *   3. The two returned bytes are scaled by 41/32 and written to +0x26 and +0x27.
 *   4. The result of a second call is ADDED as 16.16 to +0x4C and +0x50.
 *
 * The only difference between the branches: whether 0x2000000 is added to the
 * angle, the second value passed to the second call, and the mode byte written
 * at the end.
 *
 * STATUS: PARKED, 1430/1518, 88 bytes SHORT.  The allocation chain is EXACTLY
 * the same as ROM (25, 35, 33, 8, 30, 31, 40, 50, 32, 7, 6, 47 in that order),
 * and the shared epilogue at the end and the default branch fell into place
 * too.  The remaining difference is still block merging: in ROM the 0x03FFFFFF
 * mask sits in SIX separate pool words, so there are six physical blocks; we
 * have five.
 *
 * WIN 1 -- 7 and 6 are SEPARATE branches.  I had merged the two as
 * `state == 7 || state == 6`; in ROM there are two separate copies.  Splitting
 * them: 1172 -> 1312.
 *
 * WIN 2 -- EVERY BRANCH HAS ITS OWN LOCALS.  This was the decisive one.  At
 * first I gave all the branches a shared `bias`/`shift`; once the bodies were
 * byte-for-byte identical, agbcc merged case 8's block entirely with 0x1f/0x28
 * (on our side 4 bytes between `cmp #8` and `cmp #30`, in ROM 156).  ROM's
 * stack slots say the branches use separate locals: case 8 sp+8/12/16, case
 * 0x1f sp+32/36/40.  Opening three locals per branch: 1312 -> 1428.
 *
 * WIN 3 -- the default branch.  ROM sets up the actor's +4 address with
 * `adds r0, r7, #4` and tests it against zero, then writes to +34/+35 from
 * there.  Ghidra was showing this as the seemingly meaningless
 * `param_1 == -4`.
 *
 * TWO IDEAS TRIED AND REVERTED (measured, both made it WORSE):
 *
 *   1. Moving the shifts into the call argument.  ROM first computes the RAW
 *      sum of the two axes and only afterwards does the `<<23 >>24` shifts,
 *      three of them back to back; that suggested the source writes the shift
 *      directly into the argument rather than into a local.  Written that way:
 *      1430 -> 1178.  With the locals split per branch it was even worse:
 *      1164.  The idea is wrong.
 *
 *   2. Keeping the `- (rec->ox - 2)` expression in a separate local.  ROM
 *      loads `ox` and does the `-2` separately, while on our side the compiler
 *      recombines it as `+2 - ox`.  Giving it a separate local did not help on
 *      its own; since it was measured together with change 1 above, its
 *      standalone effect still has to be measured separately.
 *
 * The remaining 88 bytes are spread across the blocks (the 0x28/0x32 block
 * +44, case 8 +24, 0x1e +20, 0x1f +16) and the last block is 40 bytes LONG.
 * ROM's cross-jumping has tied the blocks into a common tail from different
 * depths; case 8 does the call setup inside itself and jumps into the tail
 * FURTHER ALONG, while 0x1f jumps to the start of the setup.  I found no known
 * lever to steer this from the source.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/state_offset.c
 */

#include "gba_types.h"

#define ANGLE_MASK 0x03FFFFFF
#define ANGLE_BIAS 0x02000000
#define SCALE_NUM  41
#define SCALE_SH   5

typedef struct Owner {
    u8 pad0[9];
    u8 kind;              /* +0x09 */
} Owner;

/* Sub-structure starting at the actor's +0x04; the facing bytes are at +34
 * and +35 inside it.  ROM first sets up this address with `adds r0, r7, #4`
 * and tests it against zero -- what Ghidra shows as `param_1 == -4` is
 * exactly this. */
typedef struct Slot {
    u8 pad0[34];
    s8 fx;                /* +0x22 (actor's +0x26) */
    s8 fy;                /* +0x23 (actor's +0x27) */
} Slot;

typedef struct Facing {
    u8 pad0[0x22];
    s8 fx;                /* +0x22 */
    s8 fy;                /* +0x23 */
} Facing;

typedef struct Nested {
    u8 pad0[0x3c];
    Facing *facing;       /* +0x3C */
} Nested;

typedef struct Bounds {
    u8 pad0[0x4b];
    s8 drop;              /* +0x4B */
} Bounds;

typedef struct Source {
    u8 pad0[0x0c];
    s32 angle;            /* +0x0C */
} Source;

typedef struct Entity {
    u8 pad0[0x10];
    Bounds *bounds;       /* +0x10 */
    u8 pad14[4];
    Source *source;       /* +0x18 */
    Nested *nested;       /* +0x1C */
} Entity;

typedef struct Record {
    u8 pad0[4];
    u8 x0;                /* +0x04 */
    u8 y0;                /* +0x05 */
    u8 x1;                /* +0x06 */
    u8 y1;                /* +0x07 */
    u8 pad8[0x10];
    u8 ox;                /* +0x18 */
    u8 oy;                /* +0x19 */
} Record;

typedef struct Actor {
    u8 pad0[0x26];
    s8 fx;                /* +0x26 */
    s8 fy;                /* +0x27 */
    u8 pad28[2];
    u8 mode;              /* +0x2A */
    u8 pad2b[0x21];
    s32 px;               /* +0x4C */
    s32 py;               /* +0x50 */
    u8 pad54[0x14];
    s32 angle;            /* +0x68 */
    u8 pad6c[0x18];
    Owner *owner;         /* +0x84 */
    u8 pad88[8];
    s32 state;            /* +0x90 */
} Actor;

extern void FUN_08029088(s32 angle, s32 dx, s32 dy, s8 *outX, s8 *outY);
extern s32 GetNegatedNested(Entity *entity);

void FUN_080260a8(Actor *actor, Entity *entity, Record *rec)
{
    s32 bias;
    s32 state;
    s32 angle;
    s32 dx;
    s32 dy;
    s32 nested;
    s32 a08, b08, c08;
    s32 a1e, b1e, c1e;
    s32 a1f, b1f, c1f;
    s32 a28, b28, c28;
    s8 ox;
    s8 oy;

    bias = 0;
    if (actor->owner->kind == 25) bias = -28;

    state = actor->state;
    if (state == 0x23) {
        angle = entity->source->angle & ANGLE_MASK;
        actor->angle = angle;
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        FUN_08029088(actor->angle >> 16, 0, -44, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        actor->mode = 0x21;
        return;
    }
    if (state == 0x21) {
        angle = entity->source->angle & ANGLE_MASK;
        actor->angle = angle;
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        FUN_08029088(actor->angle >> 16, 0, -44, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        actor->mode = 0x21;
        return;
    }

    if (state == 8) {
        nested = GetNegatedNested(entity);
        angle = (entity->source->angle + ANGLE_BIAS) & ANGLE_MASK;
        actor->angle = angle;
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        a08 = nested - (bias - 12);
        angle = actor->angle >> 16;
        b08 = a08 << 24;
        c08 = b08 >> 24;
        FUN_08029088(angle, 0, c08, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        return;
    } else if (state == 0x1e) {
        nested = GetNegatedNested(entity);
        angle = (entity->source->angle + ANGLE_BIAS) & ANGLE_MASK;
        actor->angle = angle;
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        a1e = nested - (bias - 16);
        angle = actor->angle >> 16;
        b1e = a1e << 24;
        c1e = b1e >> 24;
        FUN_08029088(angle, 0, c1e, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        return;
    } else if (state == 0x1f) {
        nested = GetNegatedNested(entity);
        angle = (entity->source->angle + ANGLE_BIAS) & ANGLE_MASK;
        actor->angle = angle;
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        a1f = nested - (bias - 12);
        angle = actor->angle >> 16;
        b1f = a1f << 24;
        c1f = b1f >> 24;
        FUN_08029088(angle, 0, c1f, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        return;
    } else if (state == 0x28 || state == 0x32) {
        nested = GetNegatedNested(entity);
        angle = (entity->source->angle + ANGLE_BIAS) & ANGLE_MASK;
        actor->angle = angle;
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        a28 = nested - (bias - 12);
        angle = actor->angle >> 16;
        b28 = a28 << 24;
        c28 = b28 >> 24;
        FUN_08029088(angle, 0, c28, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        return;
    } else if (state == 0x20) {
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(actor->angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        FUN_08029088(actor->angle >> 16, 0, entity->bounds->drop, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        actor->mode = 0x40;
        return;
    } else if (state == 7) {
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(actor->angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        FUN_08029088(actor->angle >> 16, 0, bias, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        actor->mode = 0x10;
        return;
    } else if (state == 6) {
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(actor->angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        FUN_08029088(actor->angle >> 16, 0, bias, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        actor->mode = 0x10;
        return;
    } else if (actor->state == 0x2f) {
        actor->mode = 0x10;
        return;
    } else {
        Facing *facing = entity->nested->facing;
        Slot *dst;
        s8 fx;
        s8 fy;

        fx = facing->fx;
        fy = facing->fy;
        dst = (Slot *)((u8 *)actor + 4);
        if (dst) {
            dst->fx = fx;
            dst->fy = fy;
        }
        return;
    }

}
