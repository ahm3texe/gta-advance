/* Script command: call and always succeed — 0x08059D88-0x08059D93
 *
 * The shortest handler in the block. It calls FUN_0805063C and reports success
 * unconditionally; the callee's own answer, if it has one, is discarded.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a return value.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_call_and_succeed.c
 */

#include "gba_types.h"

extern void FUN_0805063c(void);

/* 0x08059D88 */
u32 FUN_08059d88(void)
{
    FUN_0805063c();
    return 1;
}
