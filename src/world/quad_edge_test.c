/* The four-edge / two-point test - 0x0800BF18-0x0800C121 (522 bytes)
 *
 * STATUS: BYTE-MATCHING (522/522, 0 off).  It was previously 72 off.
 *
 * WHAT IT DOES
 * ------------
 * Computes the signed determinant (the 2D cross product) of two points for the
 * quad's four edges.  If the first point's value is negative on any edge it
 * returns 0 immediately.  It tracks the SMALLEST of the second point's
 * negative values and writes that edge's two endpoints into four output
 * pointers.  The `asrs #12` / `lsls #8` pairs indicate 20.12 fixed-point
 * arithmetic.  The `(radius >> 12)^2 << 8` in the prologue is a squared
 * radius.
 *
 * THE THREE MEASUREMENTS THAT CLOSED IT
 * -------------------------------------
 * (1) THE SQUARED RADIUS MUST BE ONE EXPRESSION   (72 off; the register swap
 *     was resolved)
 *     The ROM: adds r0,r2,#0 / lsls r0,#8 / mov r9,r0   <- THERE IS A COPY IN
 *     BETWEEN.  That copy is the signature of the multiplication result and
 *     the shift result being TWO SEPARATE pseudos.
 *     `radiusSq = X; radiusSq = radiusSq << 8;` produced a single pseudo and
 *     shifted in place (lsls r2,r2,#8).
 *     `radiusSq = (toSide * toSide) << 8;` produced the second pseudo.
 *     The rule 50 computation (verified with tools/dump_alloc.py):
 *       before: p31 radiusSq 11 refs /162 lifetime = 0.204  ->  ip (r12)
 *               p23 from     10 refs /151 lifetime = 0.199  ->  r9
 *       after : radiusSq 9 refs /160 lifetime = 0.169, from 0.199
 *               THE ORDER REVERSED: from -> ip, radiusSq -> r9, to -> r10.
 *     agbcc's allocation order: r0..r7, then r12(ip), then r8/r9/r10.
 *
 * (2) THE OUTERMOST `if` MUST BECOME AN EARLY RETURN   (461 off -> 11, size
 *     522)
 *     In the nested form the reload moved the prologue's `mov r4,ip` copy
 *     BEYOND the height branch and reused it at the first edge; that left r7
 *     free, so the quad[0][0] temporary was assigned to r7, giving rise to an
 *     extra `mov r1,r8` and making the function 1 instruction longer.
 *     With `if (quad[0][2] > from[2] + height) return 0;` the reload produces
 *     a FRESH `mov r7,ip` in the edge block; because r7 is occupied the
 *     temporary falls to r1 and the extra instruction disappears.  Identical
 *     to the ROM.
 *
 * (3) THE SIZE OF THE GCSE HASH TABLE   (11 off -> 0)
 *     The remaining 11 bytes were entirely STACK SLOT numbering:
 *       mine   : sp#8=quad[3][0] sp#12=quad[3][1] sp#16=quad[0][0]
 *       the ROM: sp#8=quad[0][0] sp#12=quad[3][0] sp#16=quad[3][1]
 *     On reload the slots are allocated in ASCENDING PSEUDO NUMBER order.
 *     GCSE produces the pseudos of the corner values shared between blocks
 *     (212..219); their numbers come from the expression HASH BUCKET order:
 *       bucket = (hash_base + byte_offset) mod S,  S = (instruction_count/4)|1
 *     Because `quad[0][0]` has no offset, its address is a plain `(reg 22)`;
 *     its hash is independent of the others and its bucket shifts along with S.
 *     THE MEASURED MAP (the instruction count at gcse entry -> the corner
 *     order):
 *       188..195 -> 16,24,28,36,40,0,4,12   (my old state)
 *       196..198 -> 12,16,24,28,0,36,40,4   (the ROM)
 *       200..204 -> other orders
 *     So gcse had to be entered with 196 instructions rather than 188.  The
 *     only way to add instructions WITHOUT changing the output code is
 *     REPEATED TAILS that jump2 merges by CROSS-JUMPING; cross-jumping runs
 *     AFTER gcse:
 *       - closing the four edges with `} else return 0;`: +2 instructions
 *         each, the output byte for byte THE SAME (it saturates at four, +6)
 *       - putting a second `return 1;` inside the final min block: +2
 *     Total 188 + 8 = 196.  0 off.
 *
 * PATHS RULED OUT - DO NOT TRY THESE AGAIN
 * ----------------------------------------
 * From earlier sessions (632 variants, all >= 72 off):
 *   - Parameter types: s32[4][3], a plain s32*, struct Vec*, struct Quad*
 *   - Determinant spellings: inline, intermediate variables, an inline helper
 *   - The squared radius: four different spellings, three different
 *     destination variables
 *   - decomp-permuter from two different baselines; the score stayed in the
 *     thousands
 *   - "Split the multiplication and the shift into separate variables": it
 *     jumped to 483 off
 * Paths measured and RULED OUT in this session:
 *   - The form of the height test (>=, !(>), a temporary variable, - height):
 *     none of them moved the register allocation.
 *   - Reusing ANOTHER name such as `scaled`/`squared` for `radiusSq`: the
 *     output drops to 518 bytes.  `toSide` must hold radius>>12 AND there must
 *     be a separate shift pseudo.
 *   - Reversing the multiplication operands (b*a): 522/167 off.  It does NOT
 *     affect the GCSE bucket order at all.
 *   - Splitting the edge bodies into intermediate variables: 530-546 bytes.
 *   - Separate fromSide/toSide locals per edge (rule 45): 518-546.
 *   - Computing toSide before fromSide: 530+ bytes.
 *   - Growing the instruction count with DEAD CODE (a dead copy, a dead load,
 *     a dead multiplication, a dead shift, a dead memory read - 10 different
 *     patterns): cse1 deletes them all BEFORE gcse and the instruction count
 *     stays at 188.  THIS PATH IS CLOSED.
 *   - Turning the inner `if (fromSide >= 0) {` gates into PLAIN early returns:
 *     harmless only in position 2; in positions 1, 3 and 4 it gives 526-558
 *     bytes.  The `} else return 0;` form, by contrast, does not break the
 *     output in any of the four; that is the one used.
 *   - Nested ifs instead of `&&`, extra block braces, taking the return into a
 *     variable, taking the minimum into a local, goto: they do not move the
 *     instruction count (goto -2).
 *
 * A TOOL NOTE
 * -----------
 * Measuring the instruction count at gcse entry:
 *   old_agbcc <flags> -da -o out.s in.i    ->  in the in.i.cse dump, the count
 *   of `^(insn ` + `^(jump_insn ` lines.
 * The web pseudos (212..219) and which corner offset each corresponds to are
 * read from these lines in the in.i.gcse dump:
 *   (set (reg:SI N) (mem:SI (plus (reg/v:SI 22) (const_int C))))
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/quad_edge_test.c
 */

#include "gba_types.h"

s32 FindQuadEdgeCrossing(const s32 quad[4][3], const s32 *from, const s32 *to,
                  s32 radius, s32 height, s32 *outX1, s32 *outY1,
                  s32 *outX2, s32 *outY2)
{
    s32 radiusSq;
    s32 minimum;
    s32 fromSide, toSide;

    toSide = radius >> 12;
    radiusSq = (toSide * toSide) << 8;
    minimum = 0x00FFFFFF;
    if (quad[0][2] > from[2] + height) return 0;
    {
        fromSide = ((((from[1] - quad[3][1]) >> 12) * ((quad[0][0] - quad[3][0]) >> 12) - ((from[0] - quad[3][0]) >> 12) * ((quad[0][1] - quad[3][1]) >> 12)) << 8);
        fromSide += radiusSq;
        toSide = ((((to[1] - quad[3][1]) >> 12) * ((quad[0][0] - quad[3][0]) >> 12) - ((to[0] - quad[3][0]) >> 12) * ((quad[0][1] - quad[3][1]) >> 12)) << 8);
        toSide += radiusSq;
        if (fromSide >= 0) {
        if (toSide < minimum && toSide < 0) {
            minimum = toSide;
            *outX1 = quad[3][0];
            *outY1 = quad[3][1];
            *outX2 = quad[0][0];
            *outY2 = quad[0][1];
        }
        fromSide = ((((from[1] - quad[1][1]) >> 12) * ((quad[2][0] - quad[1][0]) >> 12) - ((from[0] - quad[1][0]) >> 12) * ((quad[2][1] - quad[1][1]) >> 12)) << 8);
        fromSide += radiusSq;
        toSide = ((((to[1] - quad[1][1]) >> 12) * ((quad[2][0] - quad[1][0]) >> 12) - ((to[0] - quad[1][0]) >> 12) * ((quad[2][1] - quad[1][1]) >> 12)) << 8);
        toSide += radiusSq;
        if (fromSide >= 0) {
        if (toSide < minimum && toSide < 0) {
            minimum = toSide;
            *outX1 = quad[1][0];
            *outY1 = quad[1][1];
            *outX2 = quad[2][0];
            *outY2 = quad[2][1];
        }
        fromSide = ((((from[1] - quad[0][1]) >> 12) * ((quad[1][0] - quad[0][0]) >> 12) - ((from[0] - quad[0][0]) >> 12) * ((quad[1][1] - quad[0][1]) >> 12)) << 8);
        fromSide += radiusSq;
        toSide = ((((to[1] - quad[0][1]) >> 12) * ((quad[1][0] - quad[0][0]) >> 12) - ((to[0] - quad[0][0]) >> 12) * ((quad[1][1] - quad[0][1]) >> 12)) << 8);
        toSide += radiusSq;
        if (fromSide >= 0) {
        if (toSide < minimum && toSide < 0) {
            minimum = toSide;
            *outX1 = quad[0][0];
            *outY1 = quad[0][1];
            *outX2 = quad[1][0];
            *outY2 = quad[1][1];
        }
        fromSide = ((((from[1] - quad[2][1]) >> 12) * ((quad[3][0] - quad[2][0]) >> 12) - ((from[0] - quad[2][0]) >> 12) * ((quad[3][1] - quad[2][1]) >> 12)) << 8);
        fromSide += radiusSq;
        toSide = ((((to[1] - quad[2][1]) >> 12) * ((quad[3][0] - quad[2][0]) >> 12) - ((to[0] - quad[2][0]) >> 12) * ((quad[3][1] - quad[2][1]) >> 12)) << 8);
        toSide += radiusSq;
        if (fromSide >= 0) {
        if (toSide < minimum && toSide < 0) {
            minimum = toSide;
            *outX1 = quad[2][0];
            *outY1 = quad[2][1];
            *outX2 = quad[3][0];
            *outY2 = quad[3][1];
            return 1;
        }
        return 1;
    } else return 0;
    } else return 0;
    } else return 0;
    } else return 0;
    }
    return 0;
}
