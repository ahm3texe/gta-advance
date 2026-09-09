/* Script command: the operand into FUN_0803C840 — 0x0805B760-0x0805B76F
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_call_0803c840.c
 */

#include "gba_types.h"

extern void FUN_0803c840(u16 id);

/* 0x0805B760 */
u32 FUN_0805b760(u32 a, u32 id)
{
    FUN_0803c840(id);
    return 1;
}
