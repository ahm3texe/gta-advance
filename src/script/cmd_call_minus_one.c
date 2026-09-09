/* Script command: call FUN_08041EF8 with -1 — 0x0805A78C-0x0805A79F
 *
 * Hands the operand on with a constant -1 in the second place and always
 * answers 1. -1 is built as `movs r1,#1 / negs r1,r1`, since Thumb's movs
 * immediate is unsigned.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_call_minus_one.c
 */

#include "gba_types.h"

extern void FUN_08041ef8(u16 id, s32 value);

/* 0x0805A78C */
u32 FUN_0805a78c(u32 a, u32 id)
{
    FUN_08041ef8(id, -1);
    return 1;
}
