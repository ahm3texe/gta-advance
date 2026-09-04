/* Uc alani disari yazma — 0x0800A930-0x0800A94B
 *
 * 0x02011030 blogunun +0x30, +0x34 ve +0x38 alanlarini cagiranin verdigi
 * uc isaretciye yaziyor.
 *
 * Kural 35: sondaki `pop {r0}; bx r0` donus tipinin void oldugunu soyluyor
 * (u32 donusunde r0 canli kalir ve agbcc donus adresini r1'e alirdi).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/read_triple.c
 */

#include "gba_types.h"

/* TANIM src/misc/coord_accessors.c ve coord_more.c ile BIREBIR AYNI
   tutulmali; ayni sembole celiskili extern turu `make check`i kirar
   (TYPES-001). Alanlar dolgudan oyuldu, YERLESIM DEGISMEDI. */
typedef struct CoordBlock {
    u32 unk00;                  /* +0  */
    u32 unk04;                  /* +4  */
    u32 unk08;                  /* +8  */
    s32 second;                 /* +12 */
    s32 first;                  /* +16 */
    u8  pad14[28];
    u32 a;                      /* +0x30 */
    u32 b;                      /* +0x34 */
    u32 c;                      /* +0x38 */
    u8  pad3C[12];
    u32 unk48;                  /* +72 */
    u8  pad4C[37];
    u8  byte71;                 /* +0x71 */
} CoordBlock;

extern CoordBlock gRam02011030;

/* 0x0800A930 */
void ReadTriple(u32 *outA, u32 *outB, u32 *outC)
{
    *outA = gRam02011030.a;
    *outB = gRam02011030.b;
    *outC = gRam02011030.c;
}
