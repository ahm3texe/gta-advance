/* The quad inside test + the index of the nearest edge - 0x0800B16C (280 bytes)
 *
 * STATUS: BYTE-MATCHING (280/280, 0 off; 140/140 instructions).
 *
 * WHAT IT DOES
 * ------------
 * Computes the point's signed determinant (the 2D cross product) for each of
 * a quad's four edges.  If the value is negative on any edge the point is
 * outside and 0 is returned.  If it passes all four it finds the index of the
 * edge with the SMALLEST value and returns `(index << 8) | 1`: the low bit
 * means "inside", the high byte is "the nearest edge".  The `asrs #12` /
 * `lsls #8` pair is 20.12 fixed-point arithmetic; the `(radius >> 12)^2 << 8`
 * in the prologue is a squared radius added to the edge values as a tolerance.
 * The quad holds {x, y, z} per corner (a row stride of 12 bytes).
 *
 * Sibling file: src/world/quad_edge_test.c (0x0800BF18, byte-matching).  The
 * same quad pattern, the same edge order (3, 1, 0, 2), the same fixed-point
 * form.  This file is its "single point + index" variant.
 *
 * THERE IS NO `+= radiusSq` ON THE FIRST EDGE - AND NONE IN THE ROM EITHER
 * ------------------------------------------------------------------------
 * At 0x0800B1BA the ROM has only `lsls r0,r2,#8 / cmp r0,#0`, while on the
 * other three edges the squared radius is added with
 * `ldr r1,[sp,#0] / adds r0,r0,r1`.  There is no addition in the first block
 * of the source either.  Given the fourfold copy-paste structure in the
 * sibling file, this is an omission in the original code; adding it produces 2
 * extra instructions and does not agree with the ROM.
 *
 * THE TWO MEASUREMENTS THAT CLOSED IT
 * -----------------------------------
 * (1) THE SQUARED RADIUS MUST BE ONE EXPRESSION, WITH NO INTERMEDIATE
 *     VARIABLE  (142 -> 140 instructions)
 *     The ROM: asrs r2,r2,#12 / adds r0,r2,#0 / muls r0,r2 / lsls r0,r0,#8
 *     That is, the multiplication and the shift are THE SAME pseudo (shifted
 *     in place) while `radius>>12` is a separate pseudo.  The spellings
 *     measured:
 *       side = radius>>12; radiusSq = (side*side)<<8;   -> 111/142
 *          (an extra `adds r0,r2,#0` copy: the shift is a third pseudo)
 *       side = radius>>12; radiusSq = side*side; radiusSq <<= 8;
 *                                                      ->  91/142
 *          (the allocation flipped: radiusSq to sl, minimum to sp#0 - the
 *          REVERSE of the ROM's)
 *       radius >>= 12; radiusSq = radius*radius; radiusSq <<= 8;  -> 96/142
 *       radiusSq = (radius>>12)*(radius>>12); radiusSq <<= 8;     -> 96/142
 *       radiusSq = ((radius>>12)*(radius>>12)) << 8;   -> 133/140  CORRECT
 *     In a single expression `radius>>12` collapses to one `asrs` by CSE, the
 *     product is shifted in place, and radiusSq lands in the sp#0 slots with
 *     the minimum in sl - the ROM's allocation.
 *
 * (2) THE `return 0` BODY MUST BE AT THE END OF THE FUNCTION  (rule 49;
 *     133 -> 140)
 *     With a plain `if (side < 0) return 0;` the first three edges merge into
 *     a single tail by cross-jumping, but THE FOURTH stays inline:
 *     `bge / movs r0,#0 / b` (3 instructions), whereas the ROM has a single
 *     `blt`.  Writing the fourth block's success path first moves the tail to
 *     the end.  The sibling file's `} else return 0;` form achieves that.
 *
 * SPELLINGS RULED OUT / EQUIVALENT
 * --------------------------------
 * Forms MEASURED for the tail layout that ALL came out BYTE-MATCHING:
 *   - wrapping all four blocks in `} else return 0;`  (the one chosen; the
 *     same form as the sibling file)
 *   - wrapping only the last one / two / three blocks
 *   - `goto outside;` + `outside: return 0;` at the end of the function
 * These are indistinguishable in the output; the one consistent with the
 * sibling file was chosen.
 * RULED OUT (does not match): a plain `if (side < 0) return 0;` in all four
 * blocks (133/140), and the four radius spellings listed in (1) above.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/band_0800b16c.c
 */

#include "gba_types.h"

s32 FUN_0800b16c(const s32 quad[4][3], const s32 *point, s32 radius)
{
    s32 radiusSq;
    s32 minimum;
    s32 side;
    s32 index;

    /* It MUST be a single expression; see measurement (1) in the header. */
    radiusSq = ((radius >> 12) * (radius >> 12)) << 8;
    minimum = 0x1F400000;
    index = 0;

    /* Edge 3->0.  The ROM DOES NOT add the squared radius here. */
    side = ((((point[1] - quad[3][1]) >> 12) * ((quad[0][0] - quad[3][0]) >> 12) - ((point[0] - quad[3][0]) >> 12) * ((quad[0][1] - quad[3][1]) >> 12)) << 8);
    if (side >= 0) {
        if (side < minimum) {
            minimum = side;
            index = 3;
        }

        /* Kenar 1->2 */
        side = ((((point[1] - quad[1][1]) >> 12) * ((quad[2][0] - quad[1][0]) >> 12) - ((point[0] - quad[1][0]) >> 12) * ((quad[2][1] - quad[1][1]) >> 12)) << 8);
        side += radiusSq;
        if (side >= 0) {
            if (side < minimum) {
                minimum = side;
                index = 1;
            }

            /* Kenar 0->1 */
            side = ((((point[1] - quad[0][1]) >> 12) * ((quad[1][0] - quad[0][0]) >> 12) - ((point[0] - quad[0][0]) >> 12) * ((quad[1][1] - quad[0][1]) >> 12)) << 8);
            side += radiusSq;
            if (side >= 0) {
                if (side < minimum) {
                    minimum = side;
                    index = 0;
                }

                /* Kenar 2->3 */
                side = ((((point[1] - quad[2][1]) >> 12) * ((quad[3][0] - quad[2][0]) >> 12) - ((point[0] - quad[2][0]) >> 12) * ((quad[3][1] - quad[2][1]) >> 12)) << 8);
                side += radiusSq;
                if (side >= 0) {
                    if (side < minimum) {
                        minimum = side;
                        index = 2;
                    }
                    return (index << 8) | 1;
                } else return 0;
            } else return 0;
        } else return 0;
    } else return 0;
}
