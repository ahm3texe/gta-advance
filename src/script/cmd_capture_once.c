/* Script command: capture the session snapshot once — 0x0805B7B4-0x0805B7E3
 *
 * Does nothing and answers 0 while gFrameCounterEwram is still zero. Otherwise
 * gRam02010F4C acts as a latch: the snapshot is taken only the first time, and
 * the latch is set afterwards whether or not it was taken.
 *
 * The 1 stored into the latch and the 1 answered are the same constant in the
 * same register in the ROM, which is what writing the store just before the
 * result produces.
 *
 * Rule 71: the zero answer is FIRST here, so it is the `then` arm.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_capture_once.c
 */

#include "gba_types.h"

extern u32 gFrameCounterEwram;
extern u32 gRam02010F4C;

extern void CaptureSessionSnapshot(u32 value);

/* 0x0805B7B4 */
u32 FUN_0805b7b4(u32 value)
{
    u32 result;

    if (gFrameCounterEwram == 0) {
        result = 0;
    } else {
        if (gRam02010F4C == 0)
            CaptureSessionSnapshot(value);
        gRam02010F4C = 1;
        result = 1;
    }
    return result;
}
