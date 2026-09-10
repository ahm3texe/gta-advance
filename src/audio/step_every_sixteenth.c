/* Step once every sixteenth tick — 0x080349A4-0x080349CB
 *
 * The gate is `(gRam02000224 + 5) & 15`, so the step runs on one tick in
 * sixteen, offset by five.
 *
 * FUN_0804E310 fills a word on the stack AND answers something; the ROM keeps
 * its answer in r0 and reads the word into r1, so both go to FUN_08034DA8.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/audio/step_every_sixteenth.c
 */

#include "gba_types.h"

#define PHASE  5
#define EVERY  15

extern u32 gRam02000224;

extern u32  FUN_0804e310(u32 *out);
extern void FUN_08034da8(u32 answer, u32 value);

/* 0x080349A4 */
void FUN_080349a4(void)
{
    u32 slot;

    if (((gRam02000224 + PHASE) & EVERY) != 0)
        return;
    FUN_08034da8(FUN_0804e310(&slot), slot);
}
