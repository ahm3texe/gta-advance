/* Forward with a constant -1 — 0x08006180-0x0800618D
 *
 * The first argument is forwarded untouched; only the second is supplied. -1 is
 * `movs r1,#1 / negs r1,r1`, since Thumb's movs immediate is unsigned.
 *
 * Rule 35: `pop {r0}; bx r0` overwrites r0, so the return type is void.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/call_with_minus_one.c
 */

#include "gba_types.h"

extern void FUN_08002194(u32 a, s32 b);

/* 0x08006180 */
void FUN_08006180(u32 a)
{
    FUN_08002194(a, -1);
}
