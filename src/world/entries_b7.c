/* ShiftColumnsDown -- 0x08023B24-0x08023BB7 (148 bytes)
 *
 * Column-by-column DOWNWARD shift. For each column it reads one byte from the
 * source buffer and shifts it right by `shift`; the resulting `n` value tells
 * how many rows that column will move down. In the destination the rows sit
 * `stride` bytes apart, and moving from column to column advances by 1 byte.
 * The walker starts at the BOTTOM row of the column
 * (dest + (height-1)*stride), pulls the (height - n) rows down from above,
 * then zeroes the n rows that are left empty at the top.
 *
 * FAMILY RELATIONSHIP -- the middle one of the three:
 *
 *               step within column   column to column   direction
 *   b3 (c3c)    +stride              +1                 up    (p increases)
 *   b6 (bb8)    +1                   +stride            left  (p increases)
 *   b7 (b24)    -stride              +1                 down  (p DECREASES)
 *
 * The mirror image of b3. Even so, I did not copy the control flow from the
 * sibling; I read it from the ROM (the docs/COMPILER.md rule 49 warning);
 * something does change here as well, item 1 below.
 *
 * The signature was read from the ROM, not guessed:
 *   r0            -> dest   (pointer, not normalized)
 *   r1,r2,r3      -> there is an lsls#24/lsrs#24 pair => ALL THREE are u8
 *   [sp,#32]      -> the same normalization => u8 (stride)
 *   [sp,#36]      -> no normalization, compared against zero => pointer
 *   [sp,#40]      -> re-read on every iteration, used with asrs => int
 * The prologue `push {r4-r7,lr}` + `mov r7,sl` / `mov r6,r9` / `mov r5,r8` +
 * `push {r5,r6,r7}`: r8/r9/r10 are used as well, so EIGHT callee-saved
 * registers. The sibling b3 had seven; item 1 explains the difference. The
 * stack argument offsets being 32/36/40 (28/32/36 in b3) is a direct
 * consequence of this -- anyone copying the offsets from the sibling would
 * read the wrong parameters here. The return is `pop {r0}; bx r0` and r0 is
 * dead => void (rule 35).
 *
 * Details MEASURED from the ROM:
 *
 *  1. 0x8023B54-5C runs once BEFORE the outer loop body:
 *     `mov r0,r8` / `subs r0,#1` / `adds r1,r0,#0` / `muls r1,r5`, and the
 *     result waits in sl. That is, (height - 1) * stride has been hoisted as
 *     a loop invariant. In the source the expression is written INSIDE the
 *     loop; agbcc lifts it out by itself. The reason for the eighth
 *     callee-saved register (sl) is that this multiplication stays live
 *     across the whole loop.
 *  2. `n` is u8: the lsls#24/lsrs#24 pair at 0x8023B68. The shift is `asrs`
 *     (arithmetic) because the result of ldrb promotes to int and `shift` is
 *     an int.
 *  3. The number of rows to copy is clipped to 16 bits at 0x8023B78 with
 *     lsls#16/lsrs#16 BUT the decrement inside the loop is a plain
 *     `subs r1,#1` -- the clipping is not repeated. The variable is NOT u16;
 *     it is a (u16) cast written into a wide local (see the measurement
 *     below).
 *  4. The outer loop counter `i` is unsigned: `bcs` at 0x8023B52 and `bcc` at
 *     0x8023BA8 (rule 31). Writing `int i` would have given `bge`/`blt`.
 *  5. The walk direction decreases: `subs r2,r2,r5` / `subs r3,r3,r5`, that
 *     is p -= stride. The source pointer is q = p - n*stride
 *     (0x8023B72 `subs r3,r2,r0`).
 *  6. `dest++` (0x8023B7C-7E `movs r0,#1` / `add ip,r0`) and `i++` come out
 *     BEFORE the inner loops. This is agbcc's own motion, not the shape of
 *     the source; the increment is at the END of the body (measured, below).
 *
 * WRITINGS TRIED AND REJECTED (all measured against the 148-byte target):
 *
 *  - `u16 remain` (leaving the clipping to the type): 156 bytes, 50 off. An
 *    lsls#16/lsrs#16 is added after every `remain--`, which is not in the
 *    ROM. A wide local + an explicit (u16) cast is required (item 3).
 *  - A SEPARATE counter for the zeroing loop (`u32 rows; rows = n; ...
 *    rows--`): 148 bytes, 6 off. The separate local produces a second allocno
 *    that grabs r0 and displaces the zero constant; in the ROM both counters
 *    are r1 and the zero constant is r0. The two inner loops must share a
 *    SINGLE counter variable. Rule 50 applied in reverse: not splitting, but
 *    MERGING.
 *  - `dest++` in the MIDDLE of the body (right after the q computation):
 *    148 bytes, 12 off.
 *  - `for (i = 0; i < columns; i++, dest++)`: 148 bytes, 5 off.
 *    Rule 43's shape does NOT hold here. The ROM's early ip increment comes
 *    not from a for-increment in the source but from a separate statement at
 *    the end of the body -- the compiler moves it forward itself.
 *
 * A CLASH THAT OCCURRED DURING MEASUREMENT (for the record): this file path
 * was overwritten by another agent in the middle of the measurement, and two
 * variant runs (dest + height*stride - stride together with a wide n) looked
 * as if they matched 0x08023BB8. They do not: what got compiled was that
 * agent's ShiftRowsLeft draft. Those two variants are NOT a valid measurement
 * HERE and need to be tried again; so as not to leave a false record, I did
 * not add them to the rejected list above.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_b7.c  -> 148/148 matched
 */

#include "gba_types.h"

/* 0x08023B24 */
void ShiftColumnsDown(u8 *dest, u8 index, u8 columns, u8 height, u8 stride,
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
    for (i = 0; i < columns; i++) {
        /* Bottom row of the column. agbcc moves the multiplication out of the loop. */
        p = dest + (height - 1) * stride;
        n = *src >> shift;
        src++;
        q = p - n * stride;

        /* Pull the bottom (height - n) rows down from n rows above. */
        remain = (u16)(height - n);
        while (remain != 0) {
            *p = *q;
            p -= stride;
            q -= stride;
            remain--;
        }

        /* Zero the n rows left empty at the top. Same counter variable: see header. */
        remain = n;
        while (remain != 0) {
            *p = 0;
            p -= stride;
            remain--;
        }

        dest++;
    }
}
