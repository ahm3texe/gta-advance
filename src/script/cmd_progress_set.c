/* Script command: pass the operand straight through — 0x0805A0AC-0x0805A0BD
 *
 * The same callee as the sibling at 0x0805A024, but with the operand itself
 * rather than a value derived from the progress block. Always reports success.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_progress_set.c
 */

#include "gba_types.h"

extern void FUN_08030ae4(u16 value, u32 slot);

/* 0x0805A0AC */
u32 FUN_0805a0ac(u32 a, u32 amount)
{
    FUN_08030ae4(amount, 1);
    return 1;
}
