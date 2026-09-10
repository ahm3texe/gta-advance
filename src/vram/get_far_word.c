/* Read the +0x1F7C word — 0x08011D38-0x08011D4B
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/vram/get_far_word.c
 */

#include "gba_types.h"

#define FAR_OFFSET  0x1F7C

extern u8 gRam0201AEF0[];

/* 0x08011D38 */
u32 FUN_08011d38(void)
{
    u8 *base = gRam0201AEF0;

    return *(u32 *)(base + FAR_OFFSET);
}
