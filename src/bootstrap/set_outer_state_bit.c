/* Record one of three outcomes in gOuterState — 0x08005FE0-0x08006027
 *
 * Three of the caller's codes each set their own bit; code 8 also calls
 * FUN_08033BB4 first. Every other code is ignored. The placeholder name is
 * kept: what the codes and the bits stand for is not recorded.
 *
 * The comparison chain (`cmp #9 / beq / cmp #9 / bgt / cmp #8 / beq`) is
 * agbcc's binary search over the switch labels; the signed `bgt` is what makes
 * the parameter an `int` rather than an unsigned type.
 *
 * The three cases converge on one `orrs / str` pair in the ROM. That is the
 * compiler cross-jumping the identical tails, not something the source says.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/bootstrap/set_outer_state_bit.c
 */

#include "gba_types.h"

extern u32 gOuterState;

extern void FUN_08033bb4(void);

/* 0x08005FE0 */
void FUN_08005fe0(int code)
{
    switch (code) {
    case 8:
        FUN_08033bb4();
        gOuterState |= 1;
        break;
    case 9:
        gOuterState |= 2;
        break;
    case 12:
        gOuterState |= 4;
        break;
    }
}
