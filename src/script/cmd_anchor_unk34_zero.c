/* Script command: is the anchor's +0x34 zero — 0x0805B1AC-0x0805B1BF
 *
 * Rule 72: the result variable is introduced after the call, which is what
 * keeps this function's prologue at `push {lr}` alone.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_anchor_unk34_zero.c
 */

#include "gba_types.h"

extern s32 GetAnchorUnk34(void);

/* 0x0805B1AC */
u32 FUN_0805b1ac(void)
{
    s32 value = GetAnchorUnk34();
    u32 result = 0;

    if (value == 0)
        result = 1;
    return result;
}
