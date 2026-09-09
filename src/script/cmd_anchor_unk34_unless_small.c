/* Script command: the anchor's +0x34 unless it is small — 0x0805A114-0x0805A129
 *
 * Answers GetAnchorUnk34 when IsAnchorSmall says no, and 0 when it says yes.
 *
 * agbcc lays a two-armed if out in source order, so the arm that must come
 * first in the ROM has to be the `then` arm. Here that is the call, which is
 * why the test is written `== 0` rather than as an early exit on `!= 0`; both
 * spellings were measured, and only this one produces the ROM's `bne` over the
 * call into the zero at the end.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_anchor_unk34_unless_small.c
 */

#include "gba_types.h"

extern u32 IsAnchorSmall(void);
extern s32 GetAnchorUnk34(void);

/* 0x0805A114 */
s32 FUN_0805a114(void)
{
    s32 result;

    if (IsAnchorSmall() == 0)
        result = GetAnchorUnk34();
    else
        result = 0;
    return result;
}
