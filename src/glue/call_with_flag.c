/* Forward with a constant flag — 0x08023648-0x08023653 and 0x08023654-0x0802365F
 *
 * Two adjacent wrappers over the same callee, differing only in the third
 * argument. Rule 35: `pop {r1}; bx r1` means r0 carries a return value, so both
 * pass the callee's answer on.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/call_with_flag.c
 */

#include "gba_types.h"

extern u32 FUN_080223cc(u32 a, u32 b, u32 flag);

/* 0x08023648 */
u32 FUN_08023648(u32 a, u32 b)
{
    return FUN_080223cc(a, b, 0);
}

/* 0x08023654 */
u32 FUN_08023654(u32 a, u32 b)
{
    return FUN_080223cc(a, b, 1);
}
