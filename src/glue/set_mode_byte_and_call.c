/* Store the mode byte and dispatch — 0x08007FB8-0x08007FD9
 *
 * The first two arguments are not used at all; only the third is stored and
 * tested. Zero takes FUN_0803378C with two constant 1s, anything else takes
 * FUN_080348C0 with no argument.
 *
 * Rule 71: the FUN_0803378C arm falls through in the ROM, so it is the `then`
 * arm and the test is written `== 0`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/set_mode_byte_and_call.c
 */

#include "gba_types.h"

extern u8 gRam02001200;

extern void FUN_0803378c(u32 a, u32 b);
extern void FUN_080348c0(void);

/* 0x08007FB8 */
u32 FUN_08007fb8(u32 a, u32 b, u32 mode)
{
    gRam02001200 = mode;
    if (mode == 0)
        FUN_0803378c(1, 1);
    else
        FUN_080348c0();
    return 1;
}
