/* Ek koordinat erisimcileri — 0x0800AAD0-0x0800AAE3
 *
 * src/misc/coord_accessors.c'nin devami: `second` okuyucu (+4) ve
 * `gRam020110AC` bayragina 80 yaziyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/misc/coord_more.c
 */

#include "gba_types.h"

typedef struct CoordBlock {
    u32 unk00;                  /* +0 */
    u32 unk04;                  /* +4 */
} CoordBlock;

extern CoordBlock gRam02011030;
extern u32        gRam020110AC;

/* 0x0800AAD0 */
u32 GetCoordSecond(void)
{
    return gRam02011030.unk04;
}

/* 0x0800AADC */
void SetCoordFlag(void)
{
    gRam020110AC = 80;
}
