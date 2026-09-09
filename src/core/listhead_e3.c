/* Produce a score from the node's distance to the camera box — 0x0800D450-0x0800D52F
 *
 * What it does: it computes the SQUARED distance between the node's on-screen
 * rectangle and the camera point, halves it and clamps it to the range
 * 0..0xFFFF. The sibling function SortListByKey (src/core/listhead_e1.c) uses
 * the result as the upper half of the sort key; it is written into the node's
 * +0x1C field.
 *
 * DETAILS READ FROM THE ROM
 *
 * The box is built from the node's own position plus four offsets from the
 * owner record (+0x10):
 *     x1 = node->x + owner->left      (owner +0x00)
 *     y1 = node->y + owner->top       (owner +0x04)
 *     x2 = node->x + owner->right     (owner +0x0C)
 *     y2 = node->y + owner->bottom    (owner +0x10)
 * node->x and node->y are read with `ldrsh`, so they are SIGNED 16-bit (+0x06
 * and +0x0A). Because Thumb only has the register-offset form of ldrsh, the
 * ROM puts a `movs rN,#6` / `movs rN,#10` in front of every read -- that extra
 * instruction comes from the instruction set, not the compiler, and has no
 * counterpart in the source.
 *
 * The camera point is the INTEGER PART of the first two 16.16 values of
 * gClipBounds (0x03000014): `ldrsh r1,[r0,#2]` and `ldrsh r2,[r0,#6]`. So what
 * is read is the upper half of gClipBounds.x. This agrees exactly with the
 * "three 16.16 values" record in ram_map.csv. As the type, the `Vec3` from
 * clip_bounds.c was used VERBATIM (extern type consistency is checked).
 *
 * The distance is in point-to-rectangle form: the difference on an axis counts
 * only when the point is OUTSIDE the box; inside, that axis contributes zero.
 * The ROM writes out nine leaf blocks separately (px<x1 / px>x2 / between) x
 * (py<y1 / py>y2 / between). The comparisons are signed (`bge`/`ble`) and all
 * intermediates are `int`.
 *
 * The clamping at the end: dist += 2; if the owner record's +0x08 field
 * (radius) is POSITIVE, its square is subtracted from dist and the result
 * clamped to zero if it would go negative; then `asrs #1` (a signed halving)
 * and a ceiling of 0xFFFF.
 *
 * THREE MEASURED DETAILS (each made a one-instruction difference)
 *
 * 1) THE ORDER of `dist += 2` AND the radius read. With `dist += 2` written
 *    first in the source, `adds r1,#2` comes out BEFORE the two `ldr`s; the
 *    ROM keeps it exactly between them. Moving the read forward zeroed an
 *    8-byte difference (a one-instruction shift): 216/224 -> 224/224.
 *
 * 2) THE `box` LOCAL CANNOT BE REUSED FOR THE RADIUS. Writing
 *    `radius = box->radius;` produces 220 bytes: `box` stays live to the far
 *    end of the if tree and destroys the final `mov r4,ip` + `ldr r0,[r4,#16]`
 *    pair. The ROM RELOADS the owner pointer THERE, so the full path
 *    (`node->owner->radius`) is written in the source. Because it crosses a
 *    block boundary, agbcc's CSE does not merge it either.
 *
 * 3) THE DIFFERENCES ARE WRITTEN INSIDE THE LEAVES, NOT HOISTED IN FRONT OF
 *    THEM.
 *    Hoisting them out as `int dx = px - x1;` again gives 220 bytes: the
 *    common `subs r0,r1,r6` collapses to a single copy. The ROM computes all
 *    three separately. Written out in the leaves, the within-block CSE already
 *    produces the ROM's `subs` + `adds rX,r0,#0` + `muls` triple, and tail
 *    merging (cross-jumping) yields the shared tail at 0x0800D4D4 by itself.
 *
 * FORMS TRIED AND ELIMINATED / FOUND EQUIVALENT
 *   - `radius = box->radius`                   -> 220 bytes, ELIMINATED (see 2)
 *   - a `dx` local in front of the leaves      -> 220 bytes, ELIMINATED (see 3)
 *   - `dist += 2` before the radius read       -> 216 bytes, ELIMINATED (see 1)
 *   - Writing the camera read as
 *     `s16 *view = (s16 *)&gClipBounds; px = view[1];` ALSO matches exactly.
 *     `(s16)(gClipBounds.x >> 16)` was chosen over the pointer trick: it
 *     produces the same bytes without hiding the 16.16 meaning. agbcc already
 *     folds the shift plus narrowing into a single `ldrsh`.
 *
 * MATCH: 224/224 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/listhead_e3.c
 */

#include "gba_types.h"

/* The score's upper bound: it must fit the upper half of the sort key. */
#define SCORE_MAX 0xFFFF

/* The record the node points at from +0x10. This function reads the first
 * five words; the order field at +0x14 is used in the sibling file. */
typedef struct Owner {
    s32 left;                   /* +0x00 */
    s32 top;                    /* +0x04 */
    s32 radius;                 /* +0x08 */
    s32 right;                  /* +0x0C */
    s32 bottom;                 /* +0x10 */
} Owner;

typedef struct Node {
    u8  pad00[6];
    s16 x;                      /* +0x06 */
    u8  pad08[2];
    s16 y;                      /* +0x0A */
    u8  pad0c[4];
    Owner *owner;               /* +0x10 */
} Node;

/* The 16.16 bounding box (0x03000014); ClipBounds narrows it.
 * The type must be byte-for-byte the same as in src/core/clip_bounds.c. */
typedef struct Vec3 {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
    s32 z;                      /* +0x08 */
} Vec3;

extern Vec3 gClipBounds;

/* 0x0800D450 */
int GetNodeBoxDistance(Node *node)
{
    Owner *box;
    int x, y;
    int x1, y1, x2, y2;
    int px, py;
    int dist;
    int radius;

    x = node->x;
    box = node->owner;
    x1 = x + box->left;
    y = node->y;
    y1 = y + box->top;
    x2 = x + box->right;
    y2 = y + box->bottom;

    /* The integer part of the 16.16 camera position. */
    px = (s16)(gClipBounds.x >> 16);
    py = (s16)(gClipBounds.y >> 16);

    /* Point-to-rectangle squared distance; an axis that stays inside
     * contributes nothing. The differences are written out separately in each
     * leaf deliberately (see header, point 3). */
    if (px < x1) {
        if (py < y1)
            dist = (px - x1) * (px - x1) + (py - y1) * (py - y1);
        else if (py > y2)
            dist = (px - x1) * (px - x1) + (py - y2) * (py - y2);
        else
            dist = (px - x1) * (px - x1);
    } else if (px > x2) {
        if (py < y1)
            dist = (px - x2) * (px - x2) + (py - y1) * (py - y1);
        else if (py > y2)
            dist = (px - x2) * (px - x2) + (py - y2) * (py - y2);
        else
            dist = (px - x2) * (px - x2);
    } else {
        if (py < y1)
            dist = (py - y1) * (py - y1);
        else if (py > y2)
            dist = (py - y2) * (py - y2);
        else
            dist = 0;
    }

    /* The owner pointer is RELOADED here (see header, point 2). */
    radius = node->owner->radius;
    dist += 2;
    if (radius > 0) {
        dist -= radius * radius;
        if (dist < 0)
            dist = 0;
    }

    dist >>= 1;
    if (dist > SCORE_MAX)
        dist = SCORE_MAX;
    return dist;
}
