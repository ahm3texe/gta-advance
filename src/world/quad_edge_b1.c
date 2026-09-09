/* Polygon / point inside-outside test - 0x0800BDF8-0x0800BEBD (198 bytes)
 *
 * STATUS: BYTE-MATCHING (198/198, 0 differences).
 *        Previously: 188 bytes, 181 differences (a working copy with the
 *        wrap-around written by hand).
 *
 * !!! A BUILD PRECONDITION !!!
 * At 0x0800BE6E the ROM does `bl 0x0806C18C`; that is agbcc's Thumb __modsi3
 * routine (the `mov pc,lr` at 0x0806C188 is its divide-by-zero tail).  So the
 * `%` operator is MANDATORY in this function.  data/functions.csv knows this
 * address only under the name __modsi3, while the call symbol agbcc emits is
 * `__modsi3`.  For this file to link, ***A functions.csv RECORD NAMED
 * `__modsi3` IS NEEDED FOR THE ADDRESS 0x0806C18C*** (the existing __modsi3
 * line can be renamed, or an alias added).  Without the record build_c.py says
 * "'__modsi3' is not in data/functions.csv or data/ram_map.csv".
 * I did not add the record; touching anything under data/ was closed to this
 * task.
 * The measurement was made with a helper that adds that line temporarily in
 * memory, and ALL 198 bytes came out identical to the ROM.
 *
 * WHAT IT DOES
 * ------------
 * It checks whether `point` lies inside a closed polygon (`count` corners, each
 * a 20.12 fixed-point {x, y, z}).
 *   1. It first walks edge by edge, putting the larger of the two z values at
 *      the ends of the last edge into maxZ and the smaller into minZ.  Because
 *      the loop overwrites them on every step, what remains are the LAST edge's
 *      values; the ROM does the same, so this is not a height-range
 *      accumulation but the last edge's z bounds.
 *   2. It tests the z range with a margin of `tolerance << 7`; if outside, 0.
 *   3. For every edge it computes the 2D cross product (the signed area),
 *      scales it with `<< 8` and adds the tolerance; at the first edge that
 *      comes out negative it returns 0.  If all pass, 1.
 * The `>> 12` / `<< 8` pair is the same 20.12 fixed-point pattern as in the
 * sibling file (quad_edge_test.c).
 *
 * THE TWO MEASUREMENTS THAT CLOSED IT
 * -----------------------------------
 * (1) `%` MUST NOT BE TAKEN INTO A SEPARATE STATEMENT; IT MUST STAY INSIDE THE
 *     EXPRESSION
 *     (127 -> 2 differences; the second loop became byte-for-byte the ROM's)
 *     Writing `j = (i + 1) % count;` moves the call to the TOP of the loop
 *     body.  In the ROM the call is in the middle:
 *       ldr r5,[point,#4] / ldr r3,[poly_i,#4] / mov r9,r3
 *       subs r5,r5,r3 / asrs r5,#12      <- A is computed first
 *       adds r6,r0,#1 / adds r0,r6,#0 / mov r1,r8 / bl __modsi3
 *     That is, `(point[1]-poly[i][1])>>12` is produced BEFORE the call, which
 *     only happens while the modulo is inside the multiplication's RIGHT
 *     operand.
 *     The knock-on effect reaches the allocation: because A has to cross the
 *     call, poly[i][1] holds callee-saved r9, which in turn evicts `point` from
 *     a register into the sp#4 slots and settles `tolerance` into sl.  The ROM's
 *     `sub sp,#8` + `str r2,[sp,#4]` prologue is exactly that.  With the
 *     separate statement, `sub sp,#4` came out instead.
 *     The two separate `(i + 1) % count` writings collapse to a single call
 *     under CSE (a libcall is treated as const), and the ROM has a single `bl`
 *     as well.
 *
 * (2) ROW POINTERS ARE REQUIRED IN THE FIRST LOOP   (2 -> 0 differences)
 *     Written directly as `poly[i][2]` / `poly[i+1][2]`, gcc's loop strength
 *     reduction PRELOADS the base giv with +8:
 *       ldr r3,[sp,#0] / adds r3,#8 / ldr r2,[r3,#0] / ldr r1,[r3,#12]
 *     The ROM instead keeps the base UNPRELOADED and leaves the offsets in the
 *     memory operands:
 *       ldr r3,[sp,#0] / ldr r2,[r3,#8] / ldr r1,[r3,#20]
 *     Exactly 2 bytes of difference.  For the base giv's add_val to be 0, a giv
 *     with add_val = 0 must EXIST in the loop; taking the `poly[i]` row address
 *     into a local (`cur = poly[i];`) produces that giv, and
 *     `next = poly[i+1];` merges with it and folds into the +12 offset.
 *     That yields `[r3,#8]` and `[r3,#20]`, and `adds r3,#8` disappears.
 *
 * PATHS ELIMINATED - DO NOT RETRY THESE
 * -------------------------------------
 * A note inherited from an earlier session (CORRECTED; it was wrong):
 *   - "THE `%` OPERATOR IS FORBIDDEN: agbcc turns it into __modsi3, the symbol
 *     does not exist, the file does not compile AT ALL" -> the ABSENCE of the
 *     symbol was a missing csv record, not a restriction of the language.
 *     Wrapping by hand (`j = i+1; if (j>=count) j=0;`) gave 188 bytes / 181
 *     differences; the ROM really does make a call.
 * Paths measured and eliminated in this session:
 *   - `j = (i + 1) % count;` as a separate statement: 196 bytes, 127
 *     differences.  Because the prologue comes out as `sub sp,#4`, the
 *     allocation diverges from the start.
 *   - Writing the first loop with a single row pointer (`a[2]` and `a[5]`):
 *     200 bytes, 146 differences -- `a[5]` does not produce a separate giv and
 *     the preload comes back.
 *   - `const s32 (*edge)[3] = &poly[i];` with `edge[0][2]`/`edge[1][2]`:
 *     200 bytes, 146 differences.  A row-array pointer produces no giv.
 *   - There is NO DIFFERENCE between initialising the pointers in their
 *     declaration and assigning them separately (both give 0); the more
 *     readable form was chosen.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/quad_edge_b1.c
 *             (after the __modsi3 record above has been added)
 */

#include "gba_types.h"

s32 IsPointInPolygon(const s32 poly[][3], u16 count, const s32 *point, s32 tolerance)
{
    s32 maxZ;
    s32 minZ;
    s32 i;
    s32 side;

    maxZ = 0;
    minZ = 0;
    tolerance <<= 7;

    /* Walks edge by edge and leaves the LAST edge's z bounds: the loop
     * overwrites maxZ/minZ on every step, with no accumulation.  The ROM does
     * the same. */
    for (i = 0; i < count - 1; i++) {
        const s32 *cur;
        const s32 *next;

        /* The row addresses MUST BE LOCALS: writing poly[i][2] directly
         * preloads the base giv with +8 and produces 2 extra bytes
         * (measurement 2 in the header). */
        cur = poly[i];
        next = poly[i + 1];
        if (cur[2] > next[2]) {
            maxZ = cur[2];
            minZ = next[2];
        } else {
            maxZ = next[2];
            minZ = cur[2];
        }
    }

    /* If it is outside the height window, the edges are not examined at all. */
    if (maxZ < point[2] - tolerance) return 0;
    if (minZ > point[2] + tolerance) return 0;

    for (i = 0; i < count; i++) {
        /* `(i + 1) % count` must stay INSIDE THE EXPRESSION: taken into a
         * separate statement, the __modsi3 call moves to the top of the loop
         * and the allocation diverges from the ROM (measurement 1 in the
         * header).  The two writings collapse to a single `bl` under CSE, and
         * the ROM has a single call as well. */
        side = ((((point[1] - poly[i][1]) >> 12)
                * ((poly[(i + 1) % count][0] - poly[i][0]) >> 12)
                - ((point[0] - poly[i][0]) >> 12)
                * ((poly[(i + 1) % count][1] - poly[i][1]) >> 12)) << 8);
        side += tolerance;
        if (side < 0) return 0;
    }
    return 1;
}
