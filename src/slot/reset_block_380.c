/* Clear the 56-byte block at 0x02030380 — 0x0805102C-0x08051047
 *
 * The +0x20, +0x24 and +0x28 words are zeroed by hand and then cleared again by
 * the Memset that covers the whole block, so those three are written twice. The
 * ROM does both; it is not a redundancy to tidy away.
 *
 * The Memset's destination is the same register the three stores used, so the
 * base is not reloaded.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/slot/reset_block_380.c
 */

#include "gba_types.h"

#define BLOCK_SIZE  56

typedef struct Block380 {
    u8  pad00[0x20];
    u32 first;                  /* +0x20 */
    u32 second;                 /* +0x24 */
    u32 third;                  /* +0x28 */
    u8  pad2C[0x0C];
} Block380;                     /* 56 bytes, from the Memset length */

extern Block380 gRam02030380;

extern void *Memset(void *dest, int value, u32 count);

/* 0x0805102C */
void FUN_0805102c(void)
{
    gRam02030380.first = 0;
    gRam02030380.second = 0;
    gRam02030380.third = 0;
    Memset(&gRam02030380, 0, BLOCK_SIZE);
}
