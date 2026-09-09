/* Forward with an inserted zero argument — 0x08030874-0x08030883
 *
 * Move argument three to position four and insert zero before calling
 * FUN_0802B1E4 (ROM: adds r3,r2,#0, then movs r2,#0).
 * Rule 35: `pop {r0}; bx r0` indicates void.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/forward_with_zero.c
 */

#include "gba_types.h"

extern void FUN_0802b1e4(u32 a, u32 b, u32 c, u32 d);

/* 0x08030874 */
void ForwardWithZero(u32 a, u32 b, u32 c)
{
    FUN_0802b1e4(a, b, 0, c);
}
