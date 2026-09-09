/* Reset, then hand the signed +0x0A byte on — 0x0803214C-0x08032167
 *
 * src/progress/call_signed_byte0a.c is the same body without the leading call,
 * and reads the same field the same way: `ldrb` plus a shift pair, which is a
 * u8 field cast to s8 rather than a signed field.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/reset_then_call_byte0a.c
 */

#include "gba_types.h"

extern u8 gRam02025810[];

extern void FUN_08030290(void);
extern void FUN_080504b4(s32 value);

/* 0x0803214C */
void FUN_0803214c(void)
{
    FUN_08030290();
    FUN_080504b4((s8)gRam02025810[10]);
}
