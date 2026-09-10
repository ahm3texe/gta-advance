/* Dispatch on the mode byte — 0x08033BB4-0x08033BD7
 *
 * A non-zero gRam02001200 takes FUN_080348C0; zero takes FUN_08033F18 with the
 * word at gRam0202731C. src/glue/set_mode_byte_and_call.c is what writes that
 * byte, and it dispatches the same way round.
 *
 * Rule 71: the FUN_080348C0 arm is the one the ROM branches TO and the zero arm
 * falls through, so the test is written `!= 0`.
 *
 * FUN_08033F18 is declared with an argument here; src/world/wrap_0803378c.c
 * declares it without any, because that wrapper forwards whatever is already in
 * r0-r3 and never names it. Both describe the same entry point.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/audio/dispatch_on_mode_byte.c
 */

#include "gba_types.h"

extern u8  gRam02001200;
extern u32 gRam0202731C;

extern void FUN_080348c0(void);
extern void FUN_08033f18(u32 value);

/* 0x08033BB4 */
void FUN_08033bb4(void)
{
    if (gRam02001200 != 0)
        FUN_080348c0();
    else
        FUN_08033f18(gRam0202731C);
}
