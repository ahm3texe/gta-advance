/* Script command: hand the operand to FUN_080351AC — 0x0805AB04-0x0805AB13
 *
 * The plainest one-operand handler in the block; always answers 1.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_call_080351ac.c
 */

#include "gba_types.h"

extern void FUN_080351ac(u16 id);

/* 0x0805AB04 */
u32 FUN_0805ab04(u32 a, u32 id)
{
    FUN_080351ac(id);
    return 1;
}
