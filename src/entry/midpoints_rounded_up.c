/* Two midpoints, rounded up — 0x08031D88-0x08031DB5
 *
 * Each output is `(a + b + 1) / 2` for an unsigned pair, and the ROM computes
 * it as `(s >> 1) + (s & 1)` -- an arithmetic shift plus the low bit put back,
 * which is division by two rounding AWAY from zero rather than towards it.
 *
 * NOT BYTE-MATCHING. 4 of 25 instructions. The arithmetic is right -- both
 * midpoints, the shift, the low bit put back -- but the ROM fits the work into
 * r0 and r3-r6 while agbcc needs a fourth callee-saved register and an `ip`
 * spill, and it reads the four bytes in a different order.
 *
 * The ROM reads +0x05, +0x07, +0x06, +0x04, which IS the declaration order
 * here, and creates the 1 after the first sum, reusing the register that held
 * one of the bytes. Assigning it there was measured and is worse (3/23), so the
 * pressure is not coming from where the constant is written.
 *
 * Left in place for the arithmetic, which is the part worth having: `(a+b+1)/2`
 * computed as `(s >> 1) + (s & 1)` rounds AWAY from zero, not towards it.
 *
 * The four bytes are read in the order +0x05, +0x07, +0x06, +0x04 -- not in
 * offset order, and not in the order they are used.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/midpoints_rounded_up.c
 */

#include "gba_types.h"

typedef struct Bounds {
    u8 pad00[4];
    u8 left;                    /* +0x04 */
    u8 top;                     /* +0x05 */
    u8 right;                   /* +0x06 */
    u8 bottom;                  /* +0x07 */
} Bounds;

/* 0x08031D88 */
void FUN_08031d88(const Bounds *bounds, s32 *outX, s32 *outY)
{
    s32 top = bounds->top;
    s32 bottom = bounds->bottom;
    s32 right = bounds->right;
    s32 left = bounds->left;
    s32 one = 1;
    s32 sum;
    s32 odd;

    sum = right + left;
    sum = sum + 1;
    odd = sum;
    odd &= one;
    sum = sum >> 1;
    *outX = sum + odd;
    sum = top + bottom;
    sum = sum + 1;
    odd = sum;
    odd &= one;
    sum = sum >> 1;
    *outY = sum + odd;
}
