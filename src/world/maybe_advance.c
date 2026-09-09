/* MaybeAdvance — 0x080664F0-0x08066519
 *
 * Return 0 only when gVBlankEnabled is exactly COUNTER_MIN (=2) and the
 * counter exceeds 1; return 1 otherwise. Three siblings are in pause_helpers.c.
 *
 * BYTE-MATCHING. The previous comment and C condition (return 0 for <= 1)
 * misread the ROM's bls target. Matching the actual condition reproduces
 * the exact branch direction in natural C.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/maybe_advance.c
 */

#include "gba_types.h"

#define COUNTER_MIN    2

extern u16  gVBlankEnabled;
extern u16  gRam0200048C;

extern void FUN_080657d8(u32 arg);

/* 0x080664F0 */
u32 MaybeAdvance(void)
{
    FUN_080657d8(0);
    if (gVBlankEnabled == COUNTER_MIN && gRam0200048C > 1)
        return 0;

    return 1;
}
