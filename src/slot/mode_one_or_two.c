/* Pass 1 and 2 through, everything else to 0 — 0x0804FA7C-0x0804FA9F
 *
 * The callee's answer is narrowed to 16 bits first, and the narrowed value is
 * held as a WIDE local: a `u16` one makes agbcc keep a second copy and compare
 * the third test against it, where the ROM uses one register for all three. Zero
 * and anything above 2 answer 0; 1 and 2 answer themselves.
 *
 * The ROM tests for zero, then 1, then 2, and the zero test branches to the
 * SAME place the "above 2" fall-through reaches, so the two share one answer.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/slot/mode_one_or_two.c
 */

#include "gba_types.h"

extern u32 FUN_0804eeac(void);

/* 0x0804FA7C */
u32 FUN_0804fa7c(void)
{
    u32 mode = (u16)FUN_0804eeac();

    if (mode == 0) goto none;
    if (mode != 1) goto notOne;
    return 1;
notOne:
    if (mode == 2) goto two;
none:
    return 0;
two:
    return 2;
}
