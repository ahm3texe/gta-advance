/* Pair-index lookup — 0x0802392C-0x08023973
 *
 * Two functions. The first clears a u32. The second maps (a,b) to an index
 * 0..7: for each a, equality of b with a particular value selects the lower
 * index of the pair; otherwise select the higher one.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/pair_lookup.c
 */

#include "gba_types.h"

extern u32 gRam0202370C;

/* 0x0802392C */
void ClearRam0202370C(void)
{
    gRam0202370C = 0;
}

/* 0x08023938 */
u32 PairIndex(u32 a, u32 b)
{
    if (a == 0) {
        if (b == 0) return 0;
        return 1;
    }
    if (a == 1) {
        if (b == 1) return 2;
        return 3;
    }
    if (a == 2) {
        if (b == 2) return 4;
        return 5;
    }
    if (b == 3) return 6;
    return 7;
}
