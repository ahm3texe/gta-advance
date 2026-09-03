/* Koordinat erisimcileri — 0x0800A94C-0x0800A97F
 *
 * 0x02011030'daki blogun +12/+16 kelime ciftini okuyup yazan dort kucuk
 * yaprak fonksiyon. Cift her zaman ters sirada isleniyor: ilk parametre
 * +16'ya, ikinci parametre +12'ye karsilik geliyor.
 *
 * 0x020110AC = 0x02011030 + 0x7C olmasina ragmen ROM ayri bir literal
 * yukluyor, bu yuzden ayri sembol (docs/COMPILER.md kural 22).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/misc/coord_accessors.c
 */

#include "gba_types.h"

typedef struct CoordBlock {
    u32 unk00;              /* +0  */
    u32 unk04;              /* +4  */
    u32 unk08;              /* +8  */
    s32 second;             /* +12 */
    s32 first;              /* +16 */
    u8  pad14[52];
    u32 unk48;              /* +72 */
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
