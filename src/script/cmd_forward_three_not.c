/* Script command: forward all three operands and invert — 0x08059E48-0x08059E67
 *
 * The same call as the sibling at 0x08059E68, but the answer is turned into
 * its logical negation: 1 when FUN_0805669C returns zero, 0 otherwise.
 *
 * Rule 48: the result is materialised in a variable rather than branching to
 * two separate returns, which is what produces `movs r1,#0 / cmp r0,#0 / bne /
 * movs r1,#1 / adds r0,r1,#0`.
 *
 * The result variable is introduced AFTER the call, not before it. Declared
 * first, it is live across the call and agbcc gives it r4, which costs the
 * function a `push {r4}`; the ROM has none, so it uses a scratch register and
 * the zero is written once the call has returned.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_forward_three_not.c
 */

#include "gba_types.h"

extern u32 FUN_0805669c(u32 a, u32 b, u32 c, u32 d);

/* 0x08059E48 */
u32 FUN_08059e48(u32 a, u16 b, u16 c, u16 d)
{
    u32 answer = FUN_0805669c(a, b, c, d);
    u32 result = 0;

    if (answer == 0)
        result = 1;
    return result;
}
