/* Script command: forward with a fixed fourth argument — 0x08059AE4-0x08059AF7
 *
 * One of the handlers in the block at 0x08059A00-0x0805A600, all of which share
 * the same shape: an ignored first argument, up to three u16 operands, and a
 * return value. This one passes the two operands on with a constant 64 in the
 * place the sibling at 0x08059E68 fills from a third operand.
 *
 * The in-place `lsls r1,#16 / lsrs r1,#16` pairs are agbcc normalising the u16
 * parameters on entry (docs/COMPILER.md rule 15), not a conversion at the call.
 * The sibling handlers that convert for the callee shift ACROSS registers
 * (`lsls r0,r1,#16`) instead, which is how the two cases are told apart.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_call_four_arg.c
 */

#include "gba_types.h"

extern u32 FUN_0805669c(u32 a, u32 b, u32 c, u32 d);

/* 0x08059AE4 */
u32 FUN_08059ae4(u32 a, u16 b, u16 c)
{
    return FUN_0805669c(a, b, c, 64);
}
