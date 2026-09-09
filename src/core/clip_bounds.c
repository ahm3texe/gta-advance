/* Narrow the bounding box — 0x0800A980-0x0800A9E3
 *
 * Two passes: the lower bound is first RAISED to the box's (center - margin)
 * value, then the upper bound is LOWERED to (center + margin). That is,
 * gClipBounds is intersected with the given box.
 *
 * The third component uses the constant 0x80000 instead of an edge margin, and
 * there is AN IMPORTANT ASYMMETRY in the ROM:
 *   first pass:  `ldr r5, [pc]` loads 0xFFF80000 (negative, FROM THE POOL)
 *   second pass: `movs r2,#128 / lsls r2,#12` builds 0x80000 (BY SHIFTING)
 * Both must be written as ADDITIONS. Writing `- 0x80000` made the constant be
 * built by shifting and removed the pool word (96 bytes, the ROM has 100);
 * because 0xFFF80000 cannot be produced by shifting, the addition form forces
 * the pool.
 *
 * The base (gClipBounds) and the margin are set up BEFORE the first comparison
 * (rule 37) -- the ROM prepares both up front with `ldr r3` and `lsls r1`.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 *
 * BYTE-MATCHING (100/100).  It stayed open for a long time at 96 bytes against
 * the ROM's 100, and the obstacle came down to a SINGLE structural reason: we
 * used ONE MORE callee-saved register.
 *     ROM  : push {r4,r5,lr}       -- it loads every component into r0 AFRESH
 *     ours : push {r4,r5,r6,lr}    -- the loaded values lived in r5/r6
 * Every difference derived from that (+0x14 ldr r5 vs r0, +0x20 ldr r6 vs r0,
 * +0x22/+0x24 the constant and addition registers): in the ROM every `box->`
 * read is a short-lived temporary, while the compiler kept ours as common
 * subexpressions and lengthened their lifetimes.
 *
 * Tried and rejected at the time: `+ (s32)0xFFF80000` instead of `- 0x80000`
 * (it produced the pool load correctly but did not change the register count);
 * taking the negative constant into a separate `zlo` local (no effect).  Both
 * gave 96 bytes.
 *
 * WHAT SOLVED IT (rule 69): each component was put into its own block, so the
 * reads became six independent short-lived temporaries and r6 was freed, and
 * the repeated memory reads were done through a narrow volatile view so the
 * compiler could not cache them as a common subexpression.  Together those
 * took a difference of 26 to 0.  WARNING: the technique is LOCAL, not general
 * -- the same move made CleanupAreaTiles worse (7 -> 85).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/clip_bounds.c
 */

#include "gba_types.h"

#define Z_LOWER  ((s32)0xFFF80000)   /* loaded from the literal pool */
#define Z_UPPER  (0x80 << 12)          /* constructed with a shift */

typedef struct Vec3 {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
    s32 z;                      /* +0x08 */
} Vec3;

extern Vec3 gClipBounds;

/* 0x0800A980 */
void ClipBounds(Vec3 *box, u32 margin)
{
    Vec3 *bounds;
    s32 pad;

    bounds = &gClipBounds;
    pad = margin << 16;

    { s32 cand = box->x - pad;
      if (bounds->x < cand) bounds->x = cand; }
    { s32 cand = box->y - pad;
      if (bounds->y < cand) bounds->y = cand; }
    { s32 cand = box->z + Z_LOWER;
      if (bounds->z < cand) bounds->z = cand; }

    { s32 cand = ((volatile Vec3 *)box)->x + pad;
      if (bounds->x > cand) bounds->x = cand; }
    { s32 cand = ((volatile Vec3 *)box)->y + pad;
      if (bounds->y > cand) bounds->y = cand; }
    { s32 cand = ((volatile Vec3 *)box)->z + Z_UPPER;
      if (bounds->z > cand) bounds->z = cand; }
}
