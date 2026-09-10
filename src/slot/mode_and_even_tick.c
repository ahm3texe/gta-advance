/* Is the mode set and the tick even — 0x080510B8-0x080510D7
 *
 * Rule 33 for the parity test, and rule 72 for where it goes: the 1 is
 * materialised AFTER the call, so the answer goes into a local of its own
 * first. Written before the call the constant is live across it and the
 * function pays for a callee-saved register the ROM does not push.
 *
 * Both failures share one body at the end, reached by forward branches.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/slot/mode_and_even_tick.c
 */

#include "gba_types.h"

extern u32 GetModeCategory(void);
extern u32 FUN_08032548(void);

/* 0x080510B8 */
u32 FUN_080510b8(void)
{
    u32 tick;
    u32 odd;

    if (GetModeCategory() == 0) goto no;
    tick = FUN_08032548();
    odd = 1;
    odd &= tick;
    if (odd != 0) goto no;
    return 1;
no:
    return 0;
}
