/* Decrement four countdown counters by the frame delay — 0x08053AD8 (108 bytes)
 *
 * The ROM body carries the same ten-instruction block FOUR TIMES, unrolled by
 * hand:
 *
 *     ldr r1,[r2,#N] / cmp #0 / ble   -> is the counter positive
 *     movs #192 / lsls #18 / ldr      -> the frame delay at 0x03000000
 *     subs / str                      -> counter -= delay
 *     cmp #0 / bge / movs #0 / str    -> do not let it go below zero
 *
 * So the source writes the same statement four times as well; agbcc -O2 does
 * not unroll loops, and the number of unrolled blocks is directly the number of
 * statements in the source.
 *
 * MEASURED 1 — the base must be an EXTERN SYMBOL, not `#define ((s32 *)0x02035B30)`:
 *   Written as a constant expression, agbcc folds base+offset into FOUR
 *   SEPARATE literals (`.word 0x2035b30`, `+4`, `+8`, `+12`) and every block
 *   does its own pool read: 124 bytes, 26 instructions differing. A textbook
 *   example of rule 1.
 *   AN ELIMINATED MIDDLE PATH -- a local pointer (`s32 *p = (s32 *)0x02035B30;`):
 *   it keeps the base in a register and 50 of 50 instructions match, BUT it
 *   reduces the ROM's `ldr r0,<pool>` + `adds r2,r0,#0` pair to a single
 *   `ldr r2,<pool>` -> 104 bytes (2 bytes of copy plus 2 bytes of alignment
 *   padding short).
 *   Only a real symbol reference produces that extra copy: the symbol's address
 *   is loaded into its own pseudo (r0) and, after the first element is read
 *   from it, copied into a separate pseudo (r2) for the remaining three blocks.
 *
 * MEASURED 2 — the frame delay is RE-READ IN EVERY BLOCK:
 *   The ROM emits `movs #192 / lsls #18 / ldr r0,[r0]` in all four blocks
 *   rather than taking it into one register and carrying it: the `str`s in
 *   between kill the load (GCC 2.8.1's alias analysis treats a MEM at a
 *   constant address as clashing with the array stores). So `gFrameDelay` is
 *   written directly in the source; taking it into a local copy
 *   (`u32 d = gFrameDelay;`) would leave a single read and cut six bytes from
 *   each of three blocks.
 *
 * MEASURED 3 — 0x03000000 must be a CONSTANT EXPRESSION, not a symbol (the
 * INVERSE of rule 1):
 *   The ROM BUILDS the address with `movs #192 / lsls #18` rather than reading
 *   it from the literal pool. As an extern symbol it would go into the pool.
 *   src/interrupt/vblank_intr.c and irq_helpers.c had measured the same choice
 *   for the same address; the `#define gFrameDelay (*(u32 *)0x03000000)` form
 *   from there was reused verbatim.
 *   The reason the two addresses behave oppositely is whether they can be built
 *   by shifting: 0x03000000 = 192 << 18, while 0x02035B30 cannot.
 *
 * Rule 31: `ble`/`bge` are signed -> the counters are s32, compared with
 * `cmp #0`.
 * Rule 35: `bx lr` with no push -> a void return, a leaf function.
 *
 * Because the subtraction is `s32 - u32`, the intermediate is unsigned; the
 * ROM's `subs` instruction is the same. The clamp test is done through the
 * written-back s32 lvalue, so the comparison stays signed (`bge`).
 *
 * MATCH: 108/108 bytes (verified by assembling and linking by hand, see below).
 *
 * A REQUIRED RECORD: data/ram_map.csv has NO entry for 0x02035B30. The build
 * layer generates the `.equ` from there, so this file can only be verified with
 * `make c-match` once this line is added:
 *     0x02035B30,16,gCountdownTimers,decomp,provisional,"Four s32 countdown
 *     counters; 0x08053AD8 decrements each by gFrameDelay every frame and
 *     clamps at zero"
 * Adding the record is not allowed for me (the csv files under data/ are not
 * to be touched).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_c8.c
 */

#include "gba_types.h"

/* 0x03000000 — the frame delay written during VBlank (data/ram_map.csv:
 * gFrameDelay). Written as a constant expression; the reason is MEASURED 3. */
#define gFrameDelay (*(u32 *)0x03000000)

/* 0x02035B30 — four s32 countdown counters. The individual meaning of the
 * fields is unknown, so it was left as an unnamed array. */
extern s32 gCountdownTimers[4];

/* 0x08053AD8 */
void StepCountdownTimers(void)
{
    if (gCountdownTimers[0] > 0) {
        gCountdownTimers[0] -= gFrameDelay;
        if (gCountdownTimers[0] < 0)
            gCountdownTimers[0] = 0;
    }

    if (gCountdownTimers[1] > 0) {
        gCountdownTimers[1] -= gFrameDelay;
        if (gCountdownTimers[1] < 0)
            gCountdownTimers[1] = 0;
    }

    if (gCountdownTimers[2] > 0) {
        gCountdownTimers[2] -= gFrameDelay;
        if (gCountdownTimers[2] < 0)
            gCountdownTimers[2] = 0;
    }

    if (gCountdownTimers[3] > 0) {
        gCountdownTimers[3] -= gFrameDelay;
        if (gCountdownTimers[3] < 0)
            gCountdownTimers[3] = 0;
    }
}
