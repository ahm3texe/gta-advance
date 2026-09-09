/* Script command: is the base above the operand — 0x0805A1D8-0x0805A1F1
 *
 * Rule 72: the result variable is introduced after the call, so it lands in a
 * scratch register. The `push {r4}` this function does have is for the operand,
 * which is normalised on entry and has to survive GetBase.
 *
 * The comparison is signed (`ble`), which is what makes GetBase's answer the
 * s32 it is declared as elsewhere rather than an unsigned quantity.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_base_above.c
 */

#include "gba_types.h"

extern s32 GetBase(void);

/* 0x0805A1D8 */
u32 FUN_0805a1d8(u32 a, u16 limit)
{
    s32 base = GetBase();
    u32 result = 0;

    if (base > limit)
        result = 1;
    return result;
}
