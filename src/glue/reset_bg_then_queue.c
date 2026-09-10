/* Reset the background, step back 54, then queue — 0x08006278-0x08006299
 *
 * The argument is used twice: once as it stands and once 54 lower. The ROM
 * subtracts in place after the first call (`subs r4,#54`), so the source has
 * one variable that is reduced, not two derived values.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/reset_bg_then_queue.c
 */

#include "gba_types.h"

#define STEP_BACK  54

extern void ResetBgScrollAndAffine(u32 value);
extern void FUN_080038c4(u32 value, u32 mode);
extern void FUN_080046dc(u32 a, u32 b, u32 c);

/* 0x08006278 */
void FUN_08006278(u32 value)
{
    ResetBgScrollAndAffine(value);
    value -= STEP_BACK;
    FUN_080038c4(value, 1);
    FUN_080046dc(1, 0, 0);
}
