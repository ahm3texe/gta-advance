/* Forward with the arguments swapped and a constant 1 — 0x0800607C-0x0800608D
 *
 * The twin of src/glue/swap_two_args.c, which passes 0 instead. They sit next
 * to each other in the ROM with two bytes of padding between them, but that
 * padding belongs to this one, so the two regions are separate files.
 *
 * Rule 35: `pop {r0}; bx r0` overwrites r0, so the return type is void.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/swap_two_args_flag1.c
 */

#include "gba_types.h"

extern void FUN_08001f04(u32 a, u32 b, u32 c, u32 d);

/* 0x0800607C */
void FUN_0800607c(u32 first, u32 second, u32 third)
{
    FUN_08001f04(second, first, third, 1);
}
