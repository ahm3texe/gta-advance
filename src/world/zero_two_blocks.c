/* Iki blogu sifirlama — 0x08030C28-0x08030C4B
 *
 * gFlagsA (0x02026F38) ve gFlagsB (0x02026EF0) sekizer baytini
 * Memset ile sifirliyor.
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/zero_two_blocks.c
 */

#include "gba_types.h"

#define BLOCK_SIZE 8

extern u8 gFlagsA[];
extern u8 gFlagsB[];

extern void Memset(void *dest, u32 value, u32 size);

/* 0x08030C28 */
void ZeroTwoBlocks(void)
{
    Memset(gFlagsA, 0, BLOCK_SIZE);
    Memset(gFlagsB, 0, BLOCK_SIZE);
}
