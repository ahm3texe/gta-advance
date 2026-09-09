/* Geometry helpers — 0x0800B140 and 0x0800BDB0
 *
 * Two independent small routines; they are not adjacent in the ROM but belong
 * to the same area.
 *
 * 0x0800B140 — the dot product.  Each component is scaled with `asrs #8`
 *   BEFORE the multiplication; the classic pattern for avoiding overflow when
 *   multiplying 16.16 fixed-point values.
 *
 * 0x0800BDB0 — the box proximity test.  Compares the ABSOLUTE difference
 *   between the two points on each axis against the sum of the radii; if the
 *   difference is smaller than the total radius on all three axes it returns 1.
 *
 * PROOF OF ORDER (0x0800BDB0): for the first TWO components the ROM leaves the
 * intermediate result in the register holding `a[i]` (r1), and for the THIRD
 * it uses r0:
 *     component 0/1:  ldr r1,[..] / subs r1,r1,r0 / negs r1,r1 / subs r6,r1,r3
 *     component 2  :  ldr r1,[..] / subs r0,r1,r0 / negs r0,r0 / subs r0,r0,r3
 * That asymmetry comes from the source: the first two pass through a SHARED
 * intermediate variable on their way to the destination, while the third is
 * computed directly in the destination variable.  Writing all three the same
 * way gave a difference of 8 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/geometry.c
 */

#include "gba_types.h"

/* 0x0800B140 */
s32 DotProduct(const s32 *a, const s32 *b)
{
    return (a[0] >> 8) * (b[0] >> 8)
         + (a[1] >> 8) * (b[1] >> 8)
         + (a[2] >> 8) * (b[2] >> 8);
}

/* 0x0800BDB0 */
s32 BoxesOverlap(const s32 *a, s32 ra, const s32 *b, s32 rb)
{
    s32 reach, x, y, z;
    s32 delta;

    reach = ra + rb;

    delta = a[0] - b[0];
    if (delta < 0) delta = -delta;
    x = delta - reach;

    delta = a[1] - b[1];
    if (delta < 0) delta = -delta;
    y = delta - reach;

    /* The third component goes directly into the destination; see PROOF OF
       ORDER above. */
    z = a[2] - b[2];
    if (z < 0) z = -z;
    z = z - reach;

    if (x < 0 && y < 0 && z < 0) return 1;
    return 0;
}
