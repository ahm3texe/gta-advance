/* Script command: forward all three operands — 0x08059E68-0x08059E7D
 *
 * The plainest handler in the block: it hands its three u16 operands and the
 * ignored first argument to FUN_0805669C and returns whatever comes back. The
 * sibling at 0x08059E48 calls the same function and inverts the answer instead.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_forward_three.c
 */

#include "gba_types.h"

extern u32 FUN_0805669c(u32 a, u32 b, u32 c, u32 d);

/* 0x08059E68 */
u32 FUN_08059e68(u32 a, u16 b, u16 c, u16 d)
{
    return FUN_0805669c(a, b, c, d);
}
