/* Coordinate accessors — 0x0800A94C-0x0800A97F
 *
 * Four small leaf functions that read and write the +12/+16 word pair of the
 * block at 0x02011030. The pair is always processed in reverse order: the
 * first parameter corresponds to +16 and the second to +12.
 *
 * Even though 0x020110AC = 0x02011030 + 0x7C, the ROM loads a separate
 * literal, so it is a separate symbol (docs/COMPILER.md rule 22).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/misc/coord_accessors.c
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

/* 0x0800A94C */
void SetCoordUnk48(u32 value)
{
    gRam02011030.unk48 = value;
}

/* 0x0800A958 */
void GetCoordPair(s32 *first, s32 *second)
{
    *second = gRam02011030.second;
    *first  = gRam02011030.first;
}

/* 0x0800A968 */
void SetCoordPair(s32 first, s32 second)
{
    gRam02011030.second = second;
    gRam02011030.first  = first;
}

/* 0x0800A974 */
void ClearCoordFlag(void)
{
    gRam020110AC = 0;
}
