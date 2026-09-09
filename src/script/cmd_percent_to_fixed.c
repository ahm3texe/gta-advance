/* Script command: operand as a hundredth, in 16.16 — 0x0805A0FC-0x0805A111
 *
 * `(operand << 16) / 100` is the 16.16 fixed-point form of operand/100, which
 * is then applied to slot 3 through ApplyTwoLevels.
 *
 * The shift is a plain `lsls r0,r1,#16` with no sign or zero extension after
 * it, so the operand enters the expression at full width; a u16 parameter would
 * have been normalised on entry first. The division goes through __divsi3, the
 * SIGNED helper, which is what makes the operand an int.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_percent_to_fixed.c
 */

#include "gba_types.h"

extern void ApplyTwoLevels(s32 value, u32 slot);

/* 0x0805A0FC */
u32 FUN_0805a0fc(u32 a, int percent)
{
    ApplyTwoLevels((percent << 16) / 100, 3);
    return 1;
}
