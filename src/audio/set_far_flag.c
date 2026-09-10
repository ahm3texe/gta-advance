/* Raise the +0xBA4 flag — 0x08033BA0-0x08033BB3
 *
 * 0xBA4 is far past a Thumb immediate, so both the base and the offset come
 * through the literal pool and are added at run time.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/audio/set_far_flag.c
 */

#include "gba_types.h"

#define FLAG_OFFSET  0xBA4

extern u8 gRam02027330[];

/* 0x08033BA0 */
void FUN_08033ba0(void)
{
    u8 *base = gRam02027330;
    u32 offset = FLAG_OFFSET;

    base[offset] = 1;
}
