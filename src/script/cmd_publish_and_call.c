/* Script command: publish the argument, then call — 0x0805B0EC-0x0805B111
 *
 * Does nothing and answers 0 while gFrameCounterEwram is still zero. Once it is
 * running, the argument is stored in gRam02030328 and passed to FUN_08057A04,
 * whose answer becomes this handler's.
 *
 * Rule 71: the zero answer is at the END, so the body is the `then` arm.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_publish_and_call.c
 */

#include "gba_types.h"

extern u32 gFrameCounterEwram;
extern u32 gRam02030328;

extern u32 FUN_08057a04(u32 value);

/* 0x0805B0EC */
u32 FUN_0805b0ec(u32 value)
{
    u32 result;

    if (gFrameCounterEwram != 0) {
        gRam02030328 = value;
        result = FUN_08057a04(value);
    } else {
        result = 0;
    }
    return result;
}
