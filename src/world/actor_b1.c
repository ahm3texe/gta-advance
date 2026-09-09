/* Is there a nearby "state 2" actor in the chain -- 0x080166E8, 128 bytes.
 *
 * Walks the gUnk0202F310 chain (the link is at +0x00) from start to end.  It
 * skips itself; it skips a node whose POSE's +0x30 state byte is not 2.  For
 * the rest an octagonal approximate distance is computed and compared against
 * the 0x100000 threshold; it returns 1 at the first node that does not exceed
 * the threshold, and 0 when the chain runs out.
 *
 * DETAILS MEASURED FROM THE ROM
 *  - The pose selection is NOT THE SAME as the inline distance() in
 *    include/target_common.h: here the sub/pose selection by mask 0x30 is done
 *    only for the CHAIN node, while ITS OWN pose is read directly from +0x18
 *    (`ldr r5,[r6,#24]`, BEFORE the mask block).  There is also no null check
 *    and no DIST_MAX return.  So the shared header was not used and the
 *    computation was written locally in this file.
 *  - Rule 33: `movs r0,#48 / ldrb r2,[r4,#8] / ands r0,r2` -- the result is in
 *    the CONSTANT's register.  Writing `cur->kind & 0x30` keeps the result in
 *    the load's register; the constant must be taken into its own local
 *    (`mask`) and updated in place with `&=`.
 *  - Rule 49 / the loop form: an entry guard + a do/while that loops FROM THE
 *    BOTTOM (`cmp r4,#0 / beq end` ... `ldr r4,[r4,#0] / cmp r4,#0 / bne
 *    body`).  NOT COPIED FROM A SIBLING FILE, read from the ROM.
 *  - The reverse of rule 35: the epilogue `pop {r4,r5,r6} / pop {r1} / bx r1`
 *    -- the return address is taken into r1 because r0 carries the return
 *    value, so the signature is u32.
 *  - The threshold 0x100000 is not an immediate: the ROM builds it with
 *    `movs r0,#128 / lsls r0,#13`; writing the plain constant in the source
 *    produces the same pair, and rule 44's canonicalisation trap does not
 *    apply here (the comparison is register-to-register).
 *  - WHAT CLOSED THE LAST 7 BYTES -- THE ACCUMULATION MUST USE COMPOUND
 *    ASSIGNMENT.  The one-line
 *    `result = dx + dy - (lo>>1) - (lo>>2) + (lo>>4);` gets 56/63 instructions
 *    right but puts the accumulator in a TEMPORARY pseudo: here acc=r0 and the
 *    shift temporary=r1, in the ROM exactly the REVERSE (acc=r1, temporary
 *    r0).  Split into four separate statements (`result = dx + dy;` then the
 *    three `result -= ...`), the accumulator becomes the `result` pseudo
 *    DIRECTLY and the ROM's register allocation comes out exactly.  The same
 *    family as rule 33: WHICH variable's register the result accumulates in is
 *    chosen by the source.
 *
 * TRIED AND REJECTED
 *  - `#include "target_common.h"` + the shared `distance(cur, self)`:
 *    it produces TWO mask blocks, for a and for b, plus a null check at the
 *    top; neither is in the ROM.
 *  - The `special(cur)` form (materialising the `pose->state==2` result in a
 *    variable, rule 48): the ROM uses `cmp #2 / bne` directly here, and
 *    materialising adds an extra movs/cmp pair.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/actor_b1.c
 */

#include "gba_types.h"

#define POSE_STATE_ACTIVE 2
#define ALT_POSE_MASK     0x30
#define NEAR_LIMIT        0x100000

typedef struct Pose {
    s32 x;                  /* +0x00 */
    s32 y;                  /* +0x04 */
    u8  pad08[0x28];
    u8  state;              /* +0x30 */
} Pose;

typedef struct Actor {
    struct Actor *next;     /* +0x00 */
    u8    pad04[4];
    u8    kind;             /* +0x08 */
    u8    pad09[0x0f];
    Pose *pose;             /* +0x18 */
    u8    pad1c[4];
    u8   *alt;              /* +0x20 */
} Actor;

extern Actor *GetUnk0202F310(void);

u32 IsAnyActorNearby(Actor *self)
{
    Actor *cur;
    Pose *here;
    Pose *there;
    u32 mask;
    s32 dx;
    s32 dy;
    s32 lo;
    s32 result;

    cur = GetUnk0202F310();
    if (cur == 0) goto none;

scan:
    if (cur == self) goto next;
    if (cur->pose->state != POSE_STATE_ACTIVE) goto next;

    here = self->pose;
    mask = ALT_POSE_MASK;
    mask &= cur->kind;
    if (mask) there = (Pose *)(cur->alt + 4);
    else there = cur->pose;

    dx = there->x - here->x;
    if (dx < 0) dx = -dx;
    dy = there->y - here->y;
    if (dy < 0) dy = -dy;
    lo = dy;
    if (lo > dx) lo = dx;
    result = dx + dy;
    result -= lo >> 1;
    result -= lo >> 2;
    result += lo >> 4;
    if (result < 0) result = -result;
    if (result > NEAR_LIMIT) goto next;
    return 1;

next:
    cur = cur->next;
    if (cur != 0) goto scan;

none:
    return 0;
}
