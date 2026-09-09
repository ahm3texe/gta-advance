/* Additional coordinate accessors — 0x0800AAD0-0x0800AAE3
 *
 * A continuation of src/misc/coord_accessors.c: the `second` reader (+4), and
 * writing 80 into the `gRam020110AC` flag.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/misc/coord_more.c
 */

#include "gba_types.h"

typedef struct CoordBlock {
    u32 unk00;              /* +0  */
    u32 unk04;              /* +4  */
    u32 unk08;              /* +8  */
    s32 second;             /* +12 */
    s32 first;              /* +16 */
    u8  pad14[28];
    u32 a;                      /* +0x30 */
    u32 b;                      /* +0x34 */
    u32 c;                      /* +0x38 */
    u8  pad3C[12];
    u32 unk48;              /* +72 */
    u8  pad4C[37];
    u8  byte71;             /* +0x71 */
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
