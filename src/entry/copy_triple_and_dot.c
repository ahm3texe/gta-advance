/* Copy three words and store their negated dot product — 0x0800B0A4-0x0800B0D1
 *
 * The three words are copied straight across, and then the +0x0C word of the
 * destination gets `-a*x - b*y - c*z`, where x, y and z are the SIGNED
 * halfwords at +0x02, +0x06 and +0x0A of the third argument.
 *
 * The first term is `negs r3,r3` before the multiply, so the negation is on the
 * copied word and not on the product; the other two are subtracted.
 *
 * Every halfword read is `ldrsh` through a register offset, which is the only
 * form Thumb has AND the evidence that the three are signed.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/copy_triple_and_dot.c
 */

#include "gba_types.h"

typedef struct Triple {
    s32 a;                      /* +0x00 */
    s32 b;                      /* +0x04 */
    s32 c;                      /* +0x08 */
    s32 dot;                    /* +0x0C */
} Triple;

typedef struct Axes {
    s16 pad00;
    s16 x;                      /* +0x02 */
    s16 pad04;
    s16 y;                      /* +0x06 */
    s16 pad08;
    s16 z;                      /* +0x0A */
} Axes;

/* 0x0800B0A4 */
void FUN_0800b0a4(const Triple *src, Triple *dest, const Axes *axes)
{
    s32 a;
    s32 b;
    s32 c;

    a = src->a;
    dest->a = a;
    b = src->b;
    dest->b = b;
    c = src->c;
    dest->c = c;
    a = -a;
    a = a * axes->x;
    a = a - axes->y * b;
    a = a - axes->z * c;
    dest->dot = a;
}
