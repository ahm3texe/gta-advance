/* Copies the position + angle from the entity to the actor and adds the
 * difference between two records, rotated, to the actor's 16.16 position --
 * 0x080293F8-0x080294B1, 186 bytes, Thumb.  MATCHES (186/186).
 *
 * THE SKELETON (read from the ROM; four parameters: r0 entity, r1 actor,
 * r2/r3 records)
 *
 *   1. The three-word position at entity->source (+0x18) is COPIED to the
 *      actor's +0x4C (ldmia/stmia {r2,r4,r5}), then the angle at +0x0C is
 *      written to the actor's +0x68.
 *   2. The midpoint for each Record: (x0 + x1) - ox.  NOTE: the siblings' `-2`
 *      offset and `<<23 >>24` sign extension are ABSENT HERE; the raw
 *      difference is used.  An example of why the sibling's form must not be
 *      copied.
 *   3. The difference of the two midpoints (parameter 3 minus parameter 4)
 *      goes to FUN_0802915c.  This is NOT the FUN_08029088 of
 *      entries_c1/entries_b4: it writes FULL WORDS to the stack rather than s8
 *      (the results are read with `ldr`), and its stack slots are sp+4 / sp+8,
 *      i.e. two separate words rather than adjacent bytes.  Its signature was
 *      read from 0x0802915C: (angle, dx, dy, s32 *outX, s32 *outY), and its
 *      body rotates using the sine table at 0x08CA30D8.
 *   4. The two returned values are scaled by 328/256 and ADDED << 15 to
 *      +0x4C/+0x50 (on top of the copied position).
 *   5. The tail is exactly the DEFAULT branch of state_offset.c: the
 *      nested->facing bytes are read and written into the actor's +4
 *      sub-structure with a zero check.
 *
 * THE THREE MEASUREMENTS THAT MADE IT MATCH (184, too short -> 24 off -> 8 off
 * -> 0)
 *
 *  1. THE ANGLE GOES INTO A LOCAL.  Written as
 *     `FUN_0802915c(src->angle >> 16, ...)`, agbcc swallows the shift and
 *     produces `movs r5,#14; ldrsh r0,[r3,r5]` -- i.e. it reads the upper half
 *     of the 16.16 word DIRECTLY as a signed halfword; src therefore stays
 *     live until the call, a SECOND high register (r9) is opened up and the
 *     prologue/epilogue swell.  An `angle = src->angle;` local both produces
 *     `asrs r0,r0,#16` and kills src early: the ROM's single high register
 *     (r8 the entity, ip a temporary) comes back.
 *     THE RULE: if you do not want the narrow-read optimization, take the
 *     value into a local.
 *
 *  2. THE DIFFERENCES BECOME SEPARATE STATEMENTS.  Written inside the call
 *     arguments, agbcc does the first argument's shift first and the
 *     subtractions afterwards; the ROM's order is the reverse (the two
 *     subtractions first, then `asrs #16`).  Opening `dx`/`dy` locals moves
 *     the shift to the call site: 8 -> 0 bytes.
 *     This is the REVERSE direction of measurement 1 in entries_c1; there the
 *     shift had to stay in the argument.  The ROM's order decides the source,
 *     not the sibling.
 *
 *  3. THE FACING LOCALS ARE s32, NOT s8.  With `s8 fx`, agbcc reads both bytes
 *     with a plain `ldrb` and does not sign-extend (the storage is strb
 *     anyway).  The ROM, however, reads both signed: `ldrb + lsls#24 + asrs#24`
 *     for the first and `movs r2,#0; ldrsb r2,[r0,r2]` for the second.  The
 *     asymmetry is not in the source but a side effect of the address setup
 *     (LDRSB has no immediate offset, and because the +34 address is built in
 *     a separate register for the first read there is no zero register).  An
 *     s32 local turns both into signed reads and closes the difference.
 *
 * SPELLINGS TRIED AND REJECTED (do not delete, add to them)
 *
 *  - `FUN_0802915c(src->angle >> 16, ...)`: 184 bytes, 2 SHORT of the ROM.
 *    Measurement 1 above; caused by the `ldrsh` shortcut.
 *  - `s8 fx, fy;` locals: two sign-extension instructions disappear.
 *  - Leaving the subtractions inside the arguments: 8 bytes off, the
 *    instruction order shifts.
 *  - Writing the record order the other way round was NOT TRIED and was not
 *    needed: the ROM processes the 4th parameter (r3) first, and the source is
 *    written that way too.
 *
 * MEASURED DETAILS
 *
 *  - In the additions agbcc loads the SECOND operand FIRST (the same as
 *    entries_a3/c1): the ROM starts with `ldrb [r3,#6]` (x1) -> the source has
 *    `x0 + x1`.
 *  - The scale factor is 328, NOT 41: the lsls#2/adds/lsls#3/adds/lsls#3 chain
 *    produces 41*8, followed by `asrs #8`.  The 41/32 ratio is the same as in
 *    the siblings, but 328/256 must be written in the source; writing
 *    `41 >> 5` drops the final lsls#3.
 *  - The three-word position copy is produced by a struct assignment
 *    (`actor->pos = src->pos;`); agbcc turns the 12 bytes into an ldmia/stmia
 *    pair.
 *  - The return type is void (rule 35): the epilogue is `pop {r0}; bx r0`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_c2.c   -> 186/186
 */

#include "gba_types.h"

/* 41*8 = 328.  The ROM emits lsls#2/adds/lsls#3/adds/lsls#3/asrs#8, so the
 * factor is 328, NOT 41; the siblings' 41/32 scale is multiplied by 8 here. */
#define SCALE_NUM  328
#define SCALE_SH   8
#define POS_SH     15

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

/* The same views as in state_offset.c. */
typedef struct Slot {
    u8 pad0[34];
    s8 fx;                /* +0x22 (+0x26 in the actor) */
    s8 fy;                /* +0x23 (+0x27 in the actor) */
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

typedef struct Source {
    Vec3 pos;             /* +0x00..+0x0B */
    s32  angle;           /* +0x0C */
} Source;

typedef struct Entity {
    u8 pad0[0x18];
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
    u8   pad0[0x4c];
    Vec3 pos;             /* +0x4C..+0x57 */
    u8   pad58[0x10];
    s32  angle;           /* +0x68 */
} Actor;

extern void FUN_0802915c(s32 angle, s32 dx, s32 dy, s32 *outX, s32 *outY);

/* 0x080293F8 */
void PlaceActorFromRecords(Entity *entity, Actor *actor, Record *recA, Record *recB)
{
    Source *src;
    Facing *facing;
    Slot *dst;
    s32 ax, ay, bx, by;
    s32 dx, dy;
    s32 ox, oy;
    s32 angle;
    s32 fx, fy;

    src = entity->source;
    actor->pos = src->pos;
    angle = src->angle;
    actor->angle = angle;

    bx = (recB->x0 + recB->x1) - recB->ox;
    by = (recB->y0 + recB->y1) - recB->oy;
    ax = (recA->x0 + recA->x1) - recA->ox;
    ay = (recA->y0 + recA->y1) - recA->oy;

    dx = ax - bx;
    dy = ay - by;
    FUN_0802915c(angle >> 16, dx, dy, &ox, &oy);

    ox = (ox * SCALE_NUM) >> SCALE_SH;
    oy = (oy * SCALE_NUM) >> SCALE_SH;
    actor->pos.x += ox << POS_SH;
    actor->pos.y += oy << POS_SH;

    facing = entity->nested->facing;
    fx = facing->fx;
    fy = facing->fy;
    dst = (Slot *)((u8 *)actor + 4);
    if (dst) {
        dst->fx = fx;
        dst->fy = fy;
    }
}
