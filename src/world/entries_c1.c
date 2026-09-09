/* Rotates the midpoint of the record's two axes by the actor's angle and writes
 * it into the orientation bytes -- 0x08029390-0x080293F7, 104 bytes, Thumb.
 * MATCHES.
 *
 * Functionally the same skeleton as the first half of each branch in
 * src/world/state_offset.c (0x080260A8):
 *
 *   1. The midpoint of the two axes is extracted from the Record:
 *      (x1 + x0) - (ox - 2), then sign-extended from 9 bits with
 *      `<< 23 >> 24` and halved.  The same for the y axis with +0x07/+0x05
 *      and +0x19.
 *   2. The actor's 16.16 angle at +0x68 is converted to a table index with
 *      >> 16 and passed to FUN_08029088; the helper writes two signed bytes to
 *      the stack.
 *   3. The two returned bytes are scaled by 41/32 and written into the actor's
 *      +0x26 / +0x27 orientation bytes.
 *
 * The difference from state_offset.c's branches: the angle is NOT MASKED and
 * NOT WRITTEN BACK to the actor, and there is no second FUN_08029088 call and
 * no +0x2A mode byte.  So this looks like the extracted common core of that
 * family.
 *
 * DETAILS MEASURED FROM THE ROM
 *
 *  - The return type is void (rule 35): the epilogue is
 *    `pop {r4,r5}; pop {r0}; bx r0`, and r0 is dead after the call.
 *  - Two parameters: r0 the actor (+0x26/+0x27/+0x68) and r1 the record
 *    (+4..+7, +0x18, +0x19).  Both arrive as raw words, with no entry
 *    narrowing.
 *  - The bound fields are read with `ldrb` and there is no sign extension ->
 *    u8.  The angle is `ldr` + `asrs #16` -> a signed 16.16 word.
 *  - The output slots are sp+4 and sp+5, i.e. two ADJACENT bytes -- the same
 *    call pattern as in entries_b4.c and state_offset.c.
 *    `movs r1,#0; ldrsb r1,[r0,r1]` is the only form of an s8 read in Thumb
 *    (LDRSB has no immediate offset).
 *  - The 41/32 scale was read from the shift chain: lsls#2 / adds / lsls#3 /
 *    adds = x*41, then asrs#5.  The same constant as in state_offset.c and
 *    entries_b4.c.
 *  - `adds r1,r4,#38; strb r0,[r1,#0]`: because STRB's 5-bit immediate offset
 *    ends at 31, the addresses 38/39 are built separately.  Not a lever in the
 *    source but a consequence.
 *
 * THE THREE MEASUREMENTS THAT PRODUCED THE MATCH (41 -> 17 -> 15 -> 0 bytes
 * of difference)
 *
 *  1. THE SHIFTS GO IN THE CALL ARGUMENT, NOT IN A LOCAL.  The ROM first
 *     computes the RAW sums of the two axes, loads the angle, and only THEN
 *     performs the `<<23 >>24` pairs back to back.  Combining the sum and the
 *     shift in a single local assignment
 *     (`dx = (s32)((...) << 23) >> 24;`) inserts the shifts between the sums:
 *     41 bytes of difference.  Moving the shift into the argument brings it
 *     down to 17.
 *     NOTE: the header of state_offset.c records that the same change made
 *     things WORSE THERE (1430 -> 1178).  Not a contradiction: there the angle
 *     is additionally masked and written back to the actor, so other work
 *     comes between the shifts.
 *     Do not copy the sibling's form; read it from the ROM.
 *
 *  2. THE ADDITION OPERANDS ARE WRITTEN IN REVERSE.  agbcc loads the SECOND
 *     operand of an addition FIRST.  The ROM starts with `ldrb [r1,#6]` (x1),
 *     so the source reads `rec->x0 + rec->x1`.  The plain order leaves 2 more
 *     bytes.
 *
 *  3. THE `-2` CONSTANT IS TAKEN INTO A LOCAL (rule 44).  This was decisive.
 *     Written as `(x0 + x1) - (rec->ox - 2)`, agbcc reassociates the expression
 *     during fold: `adds r3,#2`, then `ldrb`, then `subs`.  The ROM's order is
 *     the opposite -- `ldrb r0,[r1,#24]; subs r0,#2; subs r3,r3,r0` -- i.e. the
 *     subtraction is applied to the LOADED value.  Taking the constant into a
 *     `two` local skips the fold, the constant is still emitted as an
 *     immediate, and the ROM's sequence comes out exactly: 15 -> 0.
 *     This is the answer to idea number 2 that state_offset.c's header left as
 *     "its effect on its own should be measured separately": an intermediate
 *     LOCAL does not help; an intermediate CONSTANT does.
 *
 * FORMS TRIED AND ELIMINATED (do not delete; add to this list)
 *
 *  - The intermediate local `t = rec->ox - 2; dx = sum - t;`: 22 bytes.  A
 *    separate statement does NOT PREVENT the fold, and t's lifetime also shifts
 *    the allocation.
 *    Using it together with `two` still stays at 22 -- the `t` local is
 *    harmful.
 *  - `dx = sum + (2 - rec->ox);`: 15, the same as the plain subtraction.
 *    Fold reduces both forms to the same tree.
 *  - Doing the shifts in a separate statement (`dx = (dx << 23) >> 24;`): 28
 *    bytes.
 *  - Taking the angle into an `ang` local first: no change at 17 (measured
 *    while differences 2 and 3 were still open); it was not retried once the
 *    match was reached.
 *  - Two separate locals (`twox`/`twoy`) instead of `two` also give 0; the
 *    single local was kept because it is simpler.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_c1.c   -> 104/104
 */

#include "gba_types.h"

#define SCALE_NUM  41           /* 41/32 ~ 1.28, the same as state_offset.c */
#define SCALE_SH   5

/* The same Record view as in state_offset.c: a bound pair for the two axes
 * (+0x04..+0x07) and two origin bytes (+0x18/+0x19).  The 16 bytes in between
 * are not read in this translation unit and were left as padding. */
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

/* The same offsets as in state_offset.c and entries_b4.c; in this translation
 * unit only the orientation bytes and the angle are read. */
typedef struct Actor {
    u8  pad0[0x26];
    s8  fx;               /* +0x26 */
    s8  fy;               /* +0x27 */
    u8  pad28[0x40];
    s32 angle;            /* +0x68, 16.16 */
} Actor;

/* The signature was confirmed from the ROM in state_offset.c. */
extern void FUN_08029088(s32 angle, s32 dx, s32 dy, s8 *outX, s8 *outY);

/* 0x08029390 */
void SetActorOffsetFromRecord(Actor *actor, Record *rec)
{
    s32 dx;
    s32 dy;
    s32 two;
    s8  ox;
    s8  oy;

    /* Rule 44: unless the constant is taken into a local, agbcc reassociates
     * `- (ox - 2)` into `+ 2 - ox` and the instruction order shifts. */
    two = 2;
    dx = (rec->x0 + rec->x1) - (rec->ox - two);
    dy = (rec->y0 + rec->y1) - (rec->oy - two);

    /* The shifts are deliberately in the arguments: the ROM subtracts the raw
       sums first. */
    FUN_08029088(actor->angle >> 16, (dx << 23) >> 24, (dy << 23) >> 24,
                 &ox, &oy);

    actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
    actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
}
