/* ShiftColumnsUp — 0x08023C3C-0x08023CC1 (134 bytes)
 *
 * Shifts up column by column.  For each column it reads a byte from the source
 * buffer and shifts it right by `shift`; the resulting `n` says how many rows
 * that column moves up.  In the destination the rows are `stride` bytes apart
 * (so the destination steps by stride within a column and advances 1 byte from
 * column to column).  First (height - n) rows are copied from the bottom
 * upwards, then the n rows left at the bottom are zeroed.  The classic
 * "bar/wave" drawing.
 *
 * The signature was read from the ROM, not guessed:
 *   r0            -> dest   (a pointer, not normalised)
 *   r1,r2,r3      -> they have an lsls#24/lsrs#24 pair => ALL THREE u8
 *   [sp,#28]      -> the same normalisation => u8 (stride)
 *   [sp,#32]      -> no normalisation, compared against zero => a pointer
 *   [sp,#36]      -> re-read each round, used with asrs => int
 * The prologue is `push {r4-r7,lr}` + `mov r7,r9`/`mov r6,r8`/`push {r6,r7}`,
 * so r8/r9 are used too; there are seven live values (the register table in
 * docs/COMPILER.md).  The return is `pop {r0}; bx r0` with r0 dead => void
 * (rule 35).
 *
 * Details MEASURED from the ROM and why they are written that way:
 *
 *  1. `n` is u8: the lsls#24/lsrs#24 pair at 0x8023C72.  The shift is `asrs`
 *     (arithmetic) because the ldrb's result promotes to int and `shift` is
 *     int.
 *  2. The number of rows to copy is truncated to 16 bits at 0x8023C82 with
 *     lsls#16/lsrs#16, BUT the decrement inside the loop is a plain
 *     `subs r1,#1` -- the truncation is not repeated.  So the variable is NOT
 *     u16; it is a `(u16)` conversion written into a wide local.  Writing
 *     `u16 remain` produces an extra lsls/lsrs pair after every decrement
 *     (measured: 33/134 off).
 *  3. The outer loop counter `i` is unsigned: `bcs` at 0x8023C68 and `bcc` at
 *     0x8023CB4 (rule 31).  An `int i` would have given `bge`/`blt`.
 *  4. The column advance is `dest++` at the END of the body.  The ROM computes
 *     p+1 early and parks it in `ip` (0x8023C86-88), writing it back at the
 *     bottom of the loop with `mov r4, ip` -- that falls out on its own when
 *     the increment is put at the END in the source, and does not when it is
 *     written in the middle as `dest = p + 1;` (see the rejected list).
 *
 * SPELLINGS TRIED AND REJECTED (the most valuable part; do not hit the same
 * wall):
 *
 *  - `u16 remain` (leaving the truncation to the type): 33/134 off.  An
 *     lsls#16/lsrs#16 is added after every `remain--`, and the ROM has none.
 *     A wide local + an explicit `(u16)` conversion is required (item 2
 *     above).
 *  - The position of the column advance, six variants measured (all against
 *     the 134 target):
 *        `dest = p + 1;` mid-body            -> 130 bytes (4 short)
 *        `dest++;` mid-body                  -> 130 bytes
 *        `dest = p + 1;` at the end of the body -> 126 bytes
 *        `for (i = ...; i++, dest++)`        -> 134, 12 off
 *        a separate `next = p + 1; ... dest = next` -> 134,  6 off
 *        `dest++;` at the END of the body    -> 134,  6 off  (the right form)
 *     In the mid-body form agbcc allocates `dest` directly to `ip` and
 *     eliminates the `mov r4, ip` copy at the bottom of the loop; moved to the
 *     end, `dest` stays in r4 and the ROM's copy comes out.
 *  - A separate local for the zeroing loop's counter (`rows = n; while
 *     (rows != 0) ...`): 134 bytes, 6 off -- exactly an r0/r1 swap.
 *     `dump_alloc.py` showed the reason: with 13 references / 10 lifetime the
 *     separate local rises to a priority of 3.900, becomes RANK 1 and takes
 *     the lowest free register, r0; the copy counter (3*13/13 = 3.000) comes
 *     next and takes r1.  In the ROM both are r1 and the zero constant is r0.
 *     Every variant of the same separate-local idea also stayed at 6 off:
 *     changing the declaration order, making its type `int`, writing
 *     `for (rows = n; rows != 0; rows--)`.  `u16 rows` went up to 138 bytes
 *     (truncation), and using `n` directly as the counter gave 136 (the u8
 *     decrement truncation).
 *  - Taking the zero constant into a local (`zero = 0; *p = zero;`): 5 off.
 *     Moving the constant was the wrong lever; the real problem was the
 *     counter's PRIORITY.
 *
 * THE SOLUTION: the two inner loops share a SINGLE counter variable.  That
 * produces one allocno rather than two, it falls to r1 and the zero constant
 * stays in r0 -- the ROM's allocation.  Rule 50 applied in reverse: the
 * variable had to be MERGED rather than split.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_b3.c  -> 134/134 matched
 */

#include "gba_types.h"

/* 0x08023C3C */
void ShiftColumnsUp(u8 *dest, u8 index, u8 columns, u8 height, u8 stride,
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
        p = dest;
        n = *src >> shift;
        src++;
        q = p + n * stride;

        /* Pull the top (height - n) rows up from n rows below. */
        remain = (u16)(height - n);
        while (remain != 0) {
            *p = *q;
            p += stride;
            q += stride;
            remain--;
        }

        /* Zero the n rows freed at the bottom.  The same counter variable: see
           the header. */
        remain = n;
        while (remain != 0) {
            *p = 0;
            p += stride;
            remain--;
        }

        dest++;
    }
}
