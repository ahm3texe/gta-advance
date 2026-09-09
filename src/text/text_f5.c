/* Line intersection with rectangle edges — 0x08064724-0x0806493B
 *
 * STATUS: NON-MATCHING. Output 508/536 bytes; body and flow follow the ROM,
 * with one remaining allocation decision described below. Left buildable
 * so the next investigation can continue from this state.
 *
 * BEHAVIOR, read from the ROM: intersect a line of 16.16 slope and offset
 * with all four rectangle edges. Retain intersections lying on the edge
 * segments, write the candidate NEAREST to from into hit, and return its
 * distance. Return 0 if none intersects.
 *   steep == 0: y = (slope*x >> 16)+offset; divide for edge x, multiply for y.
 *   steep != 0: x = (slope*y >> 16)+offset; exchange the roles symmetrically.
 * Both branches read the same four fields but exchange divide/multiply
 * (0x806474a-0x806478c vs 0x8064798-0x80647da). Two flag tests use x bounds
 * (fields 0/12), two use y bounds (4/16): left/right and top/bottom. Candidates
 * are (xTop,top), (xBottom,bottom), (left,yLeft), (right,yRight).
 * Field 8 is UNUSED and remains unnamed as pad08.
 *
 * CALLEES, resolved in the ROM and declared extern here:
 * - __divsi3(numerator, divisor): signed division, using the declaration
 *   already in src/save/init_save_system.c. Do not write % or / here:
 *   agbcc emits its own __divsi3 helper, whose symbol is unavailable.
 * - FUN_0800c4bc(sumOfSquares): table-based square root at 0x0800c4bc; shifts
 *   the table at 0x0834289c according to 0xffff/0xffffff/... thresholds.
 *   Call sites pass dx*dx+dy*dy and compare the result signed against
 *   0x7fffffff, supporting an s32 distance interpretation.
 *
 * MEASURED SOURCE CHOICES:
 * 1. Separate left/top locals (rule 45). Without them, flag tests read
 *    box->left/top directly: 484 bytes, even the prologue differs. With
 *    locals: 508 bytes, all nine prologue instructions match. The locals
 *    increase low-register pressure so from/hit spill at entry
 *    (str r1,[sp,#0] / str r2,[sp,#4]) as in the ROM. dump_alloc confirms
 *    spills {from,hit,yLeft,yRight}, the same NUMBER as the ROM's four slots.
 * 2. Assign left INSIDE branch B and top AFTER the branch. left crosses
 *    division calls and needs a callee-saved register (ROM r4); top does
 *    not and can use caller-saved r3. Both inside/before calls: 512 bytes,
 *    five slots; both after: 480 (worst); left inside/top after: 508, four
 *    slots. Moving top before calls consumes another callee-saved register.
 * 3. Explicit left=box->left in branch B's slope==0 arm: ROM ldr r4,[r0,#0]
 *    at 0x80647c4 really reads it there. Removing it moves the load to the
 *    merge, shortens lifetime and loses the benefit in (1).
 * 4. No %; use the direct __divsi3 call for division.
 * 5. Accumulate flags in one s32 with |=; the ROM reads/ORs/writes four
 *    times across 0x80647ea-0x8064830.
 *
 * NO DIFFERENCE, all still 508 bytes: const on from/box; u32 flags; separate
 * dx/dy/dist locals per block (already separate pseudos); sum=dx*dx+dy*dy
 * intermediate; reordered local declarations.
 *
 * REMAINING 28 BYTES — continue here instead of repeating the tests above:
 *   ROM                          ours
 *   r4 = left/best               r4 = slope/best
 *   r5 = slope                   r5 = left
 *   r6 = xTop                    r6 = box (low)
 *   r7 = xBottom                 r7 = offset (low)
 *   r8 = box (high)              r8 = flags
 *   r9 = yLeft (high)            r9 = xTop
 *   sl = offset (high)           sl = xBottom
 *   sp+0/4/8/12:                 sp+0/4/8/12:
 *     from,hit,yRight,flags        from,hit,yLeft,yRight
 * The size difference stems from allocation: ROM box=r8 and offset=sl
 * require mov rX,r8 / mov rX,sl before accesses (17+3=20 extra moves).
 * Our low-register bases allow direct ldr r0,[r6,#12]. The ROM's allocation
 * is less efficient; matching requires reproducing that pressure.
 *
 * Rule-50 priorities (dump_alloc.py --rom):
 *   slope .569 | left .341 | box .254 | offset .221 | flags .200
 *   xTop .189 | xBottom .165 | from .163 | hit .152 | yLeft .119
 * Four low callee-saved registers r4..r7 go to slope/left/box/offset in
 * priority order. In the ROM, xTop/xBottom outrank box/offset, suggesting
 * those bases had priority below .165. Investigate fewer box references
 * or a longer lifetime; the no-difference approaches above already failed.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/text/text_f5.c  -> 508/536, NON-MATCHING
 */

#include "gba_types.h"

/* Planar point; both input and output parameters use this layout. */
typedef struct Point {
    s32 x;
    s32 y;
} Point;

/* Rectangle to intersect. Field 8 is never read in the ROM and remains
 * unnamed because its meaning is unknown.
 */
typedef struct Bounds {
    s32 left;
    s32 top;
    s32 pad08;
    s32 right;
    s32 bottom;
} Bounds;

#define BOUND_TOP_HIT     1              /* valid intersection with the top edge */
#define BOUND_BOTTOM_HIT  2
#define BOUND_LEFT_HIT    4
#define BOUND_RIGHT_HIT   8
#define DIST_MAX          0x7fffffff

/* 0x0806C0F4 — signed division using the game's routine, not agbcc's
 * helper. Signature matches src/save/init_save_system.c.
 */
extern s32 __divsi3(s32 dividend, s32 divisor);

/* 0x0800C4BC — table-based square root: squared distance to distance. */
extern s32 FUN_0800c4bc(s32 squareSum);

/* 0x08064724 */
s32 FUN_08064724(s32 steep, const Point *from, Point *hit,
                 s32 slope, s32 offset, const Bounds *box)
{
    s32 xTop, xBottom, yLeft, yRight;
    s32 flags;
    s32 best, dist, dx, dy;
    s32 left, top;

    flags = 0;

    if (steep == 0) {
        /* y = (slope*x >> 16)+offset: divide to find top/bottom edge x. */
        if (slope != 0) {
            xTop    = __divsi3((box->top - offset) << 16, slope);
            xBottom = __divsi3((box->bottom - offset) << 16, slope);
        } else {
            xBottom = 0;
            xTop = 0;
        }
        top = box->top;
        left = box->left;
        yLeft  = ((slope * left) >> 16) + offset;
        yRight = ((slope * box->right) >> 16) + offset;
    } else {
        /* x = (slope*y >> 16)+offset: symmetric exchange of roles. */
        if (slope != 0) {
            left = box->left;
            yLeft  = __divsi3((left - offset) << 16, slope);
            yRight = __divsi3((box->right - offset) << 16, slope);
        } else {
            yRight = 0;
            yLeft = 0;
            left = box->left;
        }
        top = box->top;
        xTop    = ((slope * top) >> 16) + offset;
        xBottom = ((slope * box->bottom) >> 16) + offset;
    }

    /* Is the intersection on the EDGE SEGMENT? Exclude the bounds: the ROM
 * uses ble/bge, requiring strict greater-than/less-than.
 */
    if (xTop > left && xTop < box->right)
        flags |= BOUND_TOP_HIT;
    if (xBottom > left && xBottom < box->right)
        flags |= BOUND_BOTTOM_HIT;
    if (yLeft > top && yLeft < box->bottom)
        flags |= BOUND_LEFT_HIT;
    if (yRight > top && yRight < box->bottom)
        flags |= BOUND_RIGHT_HIT;

    if (flags == 0)
        return 0;

    best = DIST_MAX;

    if (flags & BOUND_TOP_HIT) {
        dx = from->x - xTop;
        dy = from->y - top;
        dist = FUN_0800c4bc(dx * dx + dy * dy);
        if (dist < best) {
            best = dist;
            hit->x = xTop;
            hit->y = box->top;
        }
    }
    if (flags & BOUND_BOTTOM_HIT) {
        dx = from->x - xBottom;
        dy = from->y - box->bottom;
        dist = FUN_0800c4bc(dx * dx + dy * dy);
        if (dist < best) {
            best = dist;
            hit->x = xBottom;
            hit->y = box->bottom;
        }
    }
    if (flags & BOUND_LEFT_HIT) {
        dx = from->x - box->left;
        dy = from->y - yLeft;
        dist = FUN_0800c4bc(dx * dx + dy * dy);
        if (dist < best) {
            best = dist;
            hit->x = box->left;
            hit->y = yLeft;
        }
    }
    if (flags & BOUND_RIGHT_HIT) {
        dx = from->x - box->right;
        dy = from->y - yRight;
        dist = FUN_0800c4bc(dx * dx + dy * dy);
        if (dist < best) {
            best = dist;
            hit->x = box->right;
            hit->y = yRight;
        }
    }

    return best;
}
