/* ShiftRowsRight -- 0x08023AA0-0x08023B21 (130 bytes)
 *
 * Row-by-row shift to the RIGHT. For each row it reads one byte from the
 * source buffer and shifts it right by `shift`; the resulting `n` value says
 * how many bytes that row will move to the right. Inside a row the bytes sit
 * CONTIGUOUSLY (stride 1), and from row to row `stride` bytes are skipped.
 * The walker starts at the RIGHTMOST end of the row (dest + length - 1),
 * pulls the (length - n) bytes from left to right, and then zeroes the n
 * bytes left empty on the left.
 *
 * PLACE WITHIN THE FAMILY -- the last of the quartet:
 *
 *              in-row/in-column step  advance    direction
 *   b3 (c3c)   +stride                +1         up      (p increases)
 *   b6 (bb8)   +1                     +stride    left    (p increases)
 *   b7 (b24)   -stride                +1         down    (p decreases)
 *   b8 (aa0)   -1                     +stride    right   (p DECREASES)
 *
 * The direct mirror twin is b6. Even so I did not copy the control flow from
 * the sibling, I read it from ROM (docs/COMPILER.md rule 49 warning) -- and
 * just as well: the loop shape came out DIFFERENT from b6's, see item 5 below.
 *
 * THE SIGNATURE WAS READ FROM ROM, NOT GUESSED:
 *   r0            -> dest   (pointer, not normalized)
 *   r1,r2,r3      -> there is an lsls#24/lsrs#24 pair => ALL THREE are u8
 *   [sp,#28]      -> the same normalization (0x8023ABE) => u8 (stride)
 *   [sp,#32]      -> no normalization, compared against zero => pointer
 *   [sp,#36]      -> re-read every iteration, used with asrs => int
 * The prologue is `push {r4-r7,lr}` + `mov r7,r9` / `mov r6,r8` / `push {r6,r7}`,
 * so r8/r9 are used as well: SEVEN callee-saved. Sibling b7 had eight, because
 * there the `n * stride` multiplication was hoisted; here the axis changed, so
 * that multiplication is GONE (in-row step 1) and so is the eighth register.
 * The stack argument offsets are therefore the same as b6's (28/32/36), not
 * b7's (32/36/40) -- anyone copying the offsets from the wrong sibling reads
 * the wrong parameters here.
 * The return is `pop {r0}; bx r0` and r0 is dead => void (rule 35).
 *
 * ROM'S ALLOCATION (different from b6's, noted for diagnosis):
 *   r7 dest   r5 i   r6 src   r4 n   r8 rows   r9 stride   ip length
 * In b6 dest was r4, rows ip, length r9, stride r8. The difference does not
 * come from a choice in the source: in b6 the row advance is at the END of the
 * body, so a separate carrier (r7) stayed live for `p + stride` and pushed
 * dest into r4. Here the advance is in the MIDDLE of the body (item 5), there
 * is no carrier, and dest stays directly in the target register. Rule 50's
 * formula gives this on its own; no intervention in the allocation was needed.
 *
 * DETAILS MEASURED FROM ROM:
 *
 *  1. `p = dest + (length - 1)`: 0x8023AD0-AD4 `mov r0,ip` / `subs r0,#1` /
 *     `adds r2,r7,r0`. There is NO `muls` like in b7, because the in-row step is 1.
 *  2. `n` is u8: the lsls#24/lsrs#24 pair at 0x8023ADC. The shift is `asrs`
 *     (arithmetic) because ldrb's result is promoted to int and `shift` is int.
 *  3. The number of bytes to copy is clipped to 16 bits at 0x8023AE8 with
 *     lsls#16/lsrs#16 BUT the decrement inside the loop is a plain `subs r1,#1`
 *     -- the clipping is not repeated. The variable is NOT u16; it is an
 *     explicit `(u16)` cast written into a wide local (same measurement in the
 *     siblings).
 *  4. The outer loop counter is unsigned: 0x8023ACE `bcs` (rule 31). Writing
 *     `int i` would have given `bge`.
 *  5. THE LOOP SHAPE DIFFERS FROM B6'S. b6 is rotated: `cmp/bcs` at the top and
 *     a copy tested a second time at the bottom with `cmp/bcc`. Here there is a
 *     single test at the top and the body branches back at 0x8023B14 with a
 *     plain `b 0x8023ACC` -- the bottom copy and the `adds r4,r7,#0` write-back
 *     are GONE, exactly 4 bytes less.
 *     What produces this is the PLACE of the `dest += stride;` statement: in
 *     the middle of the body. In b6's header this spelling stands as a path
 *     RULED OUT with "128 bytes (4 short)" -- there it is wrong, here it is the
 *     CORRECT shape. The sibling's rejected list does not apply to this file.
 *  6. In the copy loop ROM decrements q first and p second
 *     (0x8023AF8 `subs r3,#1` / 0x8023AFA `subs r2,#1`). The decrement order in
 *     the source maps straight through to this; the reversed spelling left a
 *     2-byte difference in the sibling.
 *  7. `dest += stride` (0x8023AEC) and `i++` (0x8023AEE) are emitted BEFORE the
 *     inner loops. i++ is in the `for` increment in the source; agbcc moves it
 *     forward by itself.
 *
 * ELIMINATIONS INHERITED FROM THE SIBLINGS, NOT RETRIED HERE (measured in the
 * b6/b7 headers, do not walk into the same wall):
 *  - `u16 remain`: adds lsls#16/lsrs#16 after every decrement, absent from ROM.
 *  - dropping the `(u16)` clip entirely: comes out 4 bytes short, the clip is
 *    in the source.
 *  - `int n`: the narrowing pair at 0x8023ADC disappears.
 *  - `int stride`: the entry-normalization quartet costs 14 bytes (r.15).
 *  - a SEPARATE counter local for the zeroing loop: it opens the way to a
 *    separate allocno r0/r1 swap. The two inner loops must share a SINGLE
 *    counter variable.
 *  - `int i`: `bge/blt` instead of `bcs/bcc`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_b8.c  -> 130/130 matched
 */

#include "gba_types.h"

/* 0x08023AA0 */
void ShiftRowsRight(u8 *dest, u8 index, u8 rows, u8 length, u8 stride,
                  u8 *src, int shift)
{
    u8 *p;
    u8 *q;
    u8 n;
    u32 remain;
    u32 i;

    if (src == 0)
        return;
    src += index;
    for (i = 0; i < rows; i++) {
        /* The rightmost end of the row. The in-row step is 1, so no multiply. */
        p = dest + (length - 1);
        n = *src >> shift;
        src++;
        q = p - n;

        /* Take the (length - n) bytes on the right and shift them n bytes
         * to the right, pulling from the left. The clip is explicit: see the
         * header, item 3. */
        remain = (u16)(length - n);

        /* The row advance is in the MIDDLE of the body -- this is what decides
         * the loop shape, see header item 5. In sibling b6 this spelling was wrong. */
        dest += stride;

        while (remain != 0) {
            *p = *q;
            q--;      /* ROM decrements q first; the order matters (item 6) */
            p--;
            remain--;
        }

        /* Zero the n bytes freed up on the left. Same counter variable: see header. */
        remain = n;
        while (remain != 0) {
            *p = 0;
            p--;
            remain--;
        }
    }
}
