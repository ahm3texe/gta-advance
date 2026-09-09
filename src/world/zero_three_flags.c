/* Clear three flags — 0x0803376C-0x0803378F
 *
 * Clear gCartFlag, then the words at 0x02027320 and 0x02027310.
 *
 * The ROM loads BOTH ADDRESSES FIRST (ldr r2, ldr r1), then writes in reverse
 * order. Separate writes loaded each address immediately before its store
 * (10-byte difference). Separate base locals (rule 37) reproduce both load
 * order and pool order (0x02027310 first).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/zero_three_flags.c
 */

#include "gba_types.h"

extern u8  gCartFlag;
extern u32 gRam02027310;
extern u32 gRam02027320;

/* 0x0803376C */
void ZeroThreeFlags(void)
{
    u32 *low;
    u32 *high;

    gCartFlag = 0;
    low = &gRam02027310;
    high = &gRam02027320;
    *high = 0;
    *low = 0;
}
