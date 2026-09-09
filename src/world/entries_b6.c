/* ShiftRowsLeft -- 0x08023BB8-0x08023C3B (132 bytes)
 *
 * Row-by-row shifting. For each row it reads one byte from the source buffer
 * and shifts it right by `shift`; the resulting `n` says how many bytes that
 * row will move to the left. Inside a row the bytes are CONTIGUOUS (stride 1),
 * while from row to row `stride` bytes are skipped. First (length - n) bytes
 * are pulled from right to left, then the n bytes left over on the right are
 * zeroed.
 *
 * RELATION TO ITS SIBLING -- this function is the AXIS-SWAPPED twin of
 * ShiftColumnsUp (entries_b3.c). Same signature, same control flow; the only
 * difference is that the two strides trade places:
 *
 *              stride within a row   stride from row to row
 *   b3 (c3c)   stride                1        (column by column)
 *   b6 (bb8)   1                     stride   (row by row)
 *
 * That is why the `n * stride` multiplication needed in b3 is ABSENT here: at
 * ROM 0x8023BF6 it computes q = p + n directly with `adds r3, r2, r4`. Even
 * so I did not copy the control flow from the sibling, I read it out of the
 * ROM (see the warning in docs/COMPILER.md); the two happened to come out the
 * same.
 *
 * SIGNATURE READ FROM THE ROM, NOT GUESSED:
 *   r0            -> dest   (pointer, not normalized)
 *   r1,r2,r3      -> there is an lsls#24/lsrs#24 pair => ALL THREE are u8
 *   [sp,#28]      -> same normalization (0x8023BD6) => u8 (stride)
 *   [sp,#32]      -> no normalization, compared against zero => pointer
 *   [sp,#36]      -> re-read every iteration, used with asrs => int
 * Prologue `push {r4-r7,lr}` + `mov r7,r9`/`mov r6,r8`/`push {r6,r7}`, so
 * r8/r9 are in use too. The return is `pop {r0}; bx r0` and r0 is dead =>
 * void (rule 35).
 *
 * THE ROM'S ALLOCATION (different from b3's, noted for diagnosis):
 *   r4 dest (then n)  r5 i  r6 src  r7 next  r8 stride  r9 length  ip rows
 * In b3 stride was used in both inner loops, so it fell into r5 and the row
 * count fell into r8. Here stride has a single reference (p + stride), so it
 * stays in r8 and the outer loop bound moves up into ip. That is, the
 * allocation difference does not come from a choice in the source but from the
 * axis swap changing the reference counts -- rule 50's formula gives this on
 * its own, no intervention was needed.
 *
 * DETAILS MEASURED FROM THE ROM:
 *
 *  1. `n` is u8: the lsls#24/lsrs#24 pair at 0x8023BF0. The shift is `asrs`
 *     (arithmetic) because ldrb's result is promoted to int and `shift` is int.
 *  2. The number of bytes to copy is truncated to 16 bits at 0x8023BFC with
 *     lsls#16/lsrs#16, BUT the decrement inside the loop is a plain
 *     `subs r1,#1` -- the truncation is not repeated. So the variable is NOT
 *     u16; it is an explicit `(u16)` cast written into a wide local.
 *  3. The outer loop counter is unsigned: `bcs` at 0x8023BE6 and `bcc` at
 *     0x8023C2E (rule 31).
 *  4. In the copy loop the ROM increments q first, then p
 *     (0x8023C0E `adds r3,#1` / 0x8023C10 `adds r2,#1`). The increment order
 *     in the source maps straight through to here -- in the sibling the order
 *     was the other way round (p first).
 *  5. The row advance is at the END of the body. The ROM computes p+stride
 *     early and parks it in r7 (0x8023C02), then writes it back at the bottom
 *     of the loop with `adds r4,r7,#0`; this falls out on its own once the
 *     increment is placed at the end.
 *
 * SPELLINGS TRIED AND REJECTED (the most valuable part, do not walk into the
 * same wall):
 *
 *  - `p++; q++;` order in the copy loop (the reverse of the ROM's): 2/132 off.
 *    The only knob is the SOURCE ORDER of the increments; nothing else changes.
 *  - `int i` (signed outer counter): 2/132 off, the two branches become
 *    `bge/blt` instead of `bcs/bcc`. A direct confirmation of rule 31.
 *  - The placement of the row advance, four variants measured (all against the
 *    132-byte target):
 *        `dest = p + stride;` mid-body            -> 128 bytes (4 short)
 *        `for (i = ...; i++, dest += stride)`     -> 132, off  6
 *        separate `next = p + stride; ... dest = next` -> 132, off 11
 *        `dest += stride;` at the END of the body -> 132, off  0  (correct form)
 *    In the middle form agbcc allocates `dest` straight into the destination
 *    register and eliminates the copy at the bottom of the loop; moving it to
 *    the end brings the copy back.
 *  - A SEPARATE counter local for the zeroing loop: 132 bytes, 20 off. The
 *    same class of problem came up in the sibling as well (6 off there): the
 *    separate allocno raises the priority and opens the way to an r0/r1 swap.
 *    The two inner loops must share a SINGLE counter variable.
 *  - `u16 remain` (leaving the truncation to the type): 140 bytes, 50 off. An
 *    lsls#16/lsrs#16 is added after every `remain--`, which the ROM does not
 *    have.
 *  - Dropping the `(u16)` truncation altogether: 128 bytes (4 short). The
 *    truncation really is in the source, not in the type.
 *  - `int n`: 128 bytes. The narrowing pair at 0x8023BF0 disappears.
 *  - `int stride` (that is, [sp,#28] as a wide parameter): 118 bytes. Along
 *    with the normalization quartet in the prologue, 14 bytes go away
 *    (rule 15).
 *
 * FORMS MEASURED AS EQUIVALENT (both match exactly, the choice is style):
 *  - `*p++ = *q++;` as a single statement -- the same code as writing the
 *    three steps out explicitly. The explicit spelling was preferred for
 *    stylistic consistency with the sibling.
 *  - `q = dest + n;` (dest as the base instead of p) -- since p == dest at
 *    that point, CSE merges the two.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_b6.c  -> 132/132 matched
 */

#include "gba_types.h"

/* 0x08023BB8 */
void ShiftRowsLeft(u8 *dest, u8 index, u8 rows, u8 length, u8 stride,
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
        p = dest;
        n = *src >> shift;
        src++;
        q = p + n;

        /* Pull the left-hand (length - n) bytes n bytes in from the right and
         * shift them left. The truncation is explicit: see header, item 2. */
        remain = (u16)(length - n);
        while (remain != 0) {
            *p = *q;
            q++;      /* the ROM increments q first; order matters (item 4) */
            p++;
            remain--;
        }

        /* Zero the n bytes vacated on the right. Same counter variable: see header. */
        remain = n;
        while (remain != 0) {
            *p = 0;
            p++;
            remain--;
        }

        dest += stride;   /* row advance at the END of the body (item 5) */
    }
}
