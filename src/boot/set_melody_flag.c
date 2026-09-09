/* Set the melody flag to 6 — 0x08063B98-0x08063BA3
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/boot/set_melody_flag.c
 */

#include "gba_types.h"

#define MELODY_VALUE  6

extern u8 gRam0201AA90;

/* 0x08063B98 */
void FUN_08063b98(void)
{
    gRam0201AA90 = MELODY_VALUE;
}
