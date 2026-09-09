/* Script command: forward with two fixed arguments — 0x0805A12C-0x0805A13F
 *
 * Hands the operand to FUN_0803ABC4 with a constant 3 and 0 after it, and
 * always reports success.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_call_three_fixed.c
 */

#include "gba_types.h"

extern void FUN_0803abc4(u16 id, u32 b, u32 c);

/* 0x0805A12C */
u32 FUN_0805a12c(u32 a, u32 id)
{
    FUN_0803abc4(id, 3, 0);
    return 1;
}
