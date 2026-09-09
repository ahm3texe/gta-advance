/* Script command: call FUN_08030DD8 and succeed — 0x0805A500-0x0805A50B
 *
 * Takes no operand and always answers 1; the same shape as the handler at
 * 0x08059D88.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_call_08030dd8.c
 */

#include "gba_types.h"

extern void FUN_08030dd8(void);

/* 0x0805A500 */
u32 FUN_0805a500(void)
{
    FUN_08030dd8();
    return 1;
}
