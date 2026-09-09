/* Copy a field — 0x0805AC44-0x0805AC4F
 *
 * Copy +20 to +12 in the gRam02025810 block.
 * Its sibling BumpOrReset is in src/world/bump_or_reset.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/counter_saturate.c
 */

#include "gba_types.h"
#include "ram_symbols.h"


typedef struct Pack {
    u8  pad0000[12];
    u32 dest;                   /* +0x0C */
    u8  pad10[4];
    u32 src;                    /* +0x14 */
} Pack;

/* 0x0805AC44 */
void CopySrcToDest(void)
{
    Pack *pack = (Pack *)gRam02025810;
    pack->dest = pack->src;
}
