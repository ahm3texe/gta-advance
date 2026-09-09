/* Clear the 416-byte block at 0x02035CF0 — 0x080621D8-0x080621EF
 *
 * 416 is `movs r2,#208 / lsls r2,#1`, since Thumb's movs immediate stops at
 * 255. The clear is the only thing known about the block, and is what
 * data/ram_map.csv records its size from.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/session/reset_large_block.c
 */

#include "gba_types.h"

#define LARGE_BLOCK_SIZE  (208 << 1)

extern u8 gRam02035CF0[];

extern void *Memset(void *dest, int value, u32 count);

/* 0x080621D8 */
void FUN_080621d8(void)
{
    Memset(gRam02035CF0, 0, LARGE_BLOCK_SIZE);
}
