/* Set gRam0201AEE0 to 1 — 0x08011FAC-0x08011FB7
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/vram/set_halfword_one.c
 */

#include "gba_types.h"

extern u16 gRam0201AEE0;

/* 0x08011FAC */
void FUN_08011fac(void)
{
    gRam0201AEE0 = 1;
}
