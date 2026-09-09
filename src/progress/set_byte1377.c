/* Write 20 into the +0x1377 byte — 0x08032318-0x0803232B
 *
 * Both the base and the offset come through the literal pool, since 0x1377 is
 * far past a Thumb immediate; the ROM adds them at run time rather than folding
 * them into one constant, so the offset is a local too.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/set_byte1377.c
 */

#include "gba_types.h"

#define BYTE_OFFSET  0x1377
#define BYTE_VALUE   20

extern u8 gRam02025810[];

/* 0x08032318 */
void FUN_08032318(void)
{
    u8 *base = gRam02025810;
    u32 offset = BYTE_OFFSET;

    base[offset] = BYTE_VALUE;
}
