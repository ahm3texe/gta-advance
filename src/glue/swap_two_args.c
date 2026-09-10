/* Forward with the first two arguments swapped — 0x08006090-0x080060A1
 *
 * The third is passed through untouched and a constant 0 is added as a fourth.
 * The swap needs the scratch copy the ROM makes (`adds r3,r0,#0`), which is
 * what naming both parameters in the other order gives.
 *
 * Rule 35: `pop {r0}; bx r0` overwrites r0, so the return type is void.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/swap_two_args.c
 */

#include "gba_types.h"

extern void FUN_08001f04(u32 a, u32 b, u32 c, u32 d);

/* 0x08006090 */
void FUN_08006090(u32 first, u32 second, u32 third)
{
    FUN_08001f04(second, first, third, 0);
}
