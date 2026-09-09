/* Script commands: the operand into FUN_0803ABC4 with a fixed kind
 * 0x0805B0C4-0x0805B0D7 and 0x0805B0D8-0x0805B0EB
 *
 * Two handlers that differ only in the constant they pass, 1 and 2. The one at
 * 0x0805A12C passes 3 to the same callee; it is in its own file because it sits
 * far enough away that nothing links the three except the callee.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_call_kind.c
 */

#include "gba_types.h"

extern void FUN_0803abc4(u16 id, u32 kind, u32 c);

/* 0x0805B0C4 */
u32 FUN_0805b0c4(u32 a, u32 id)
{
    FUN_0803abc4(id, 1, 0);
    return 1;
}

/* 0x0805B0D8 */
u32 FUN_0805b0d8(u32 a, u32 id)
{
    FUN_0803abc4(id, 2, 0);
    return 1;
}
