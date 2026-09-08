/* Iki tablo imlecini ilerletip 16-bit soz uret — 0x08032548.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x08032548.json. */
#include "gba_types.h"

extern u32 gRam02027300;
extern u32 gRam020272F8;
extern u32 gRam020272F4;
extern const u8 gRom08CA6608[];
u32 FUN_08032548(void)
{
    u32 high,low;
    high = (gRam02027300 + 3) & 1023;
    gRam02027300 = high;
    low = (gRam020272F8 + 1) & 1023;
    gRam020272F8 = low;
    gRam020272F4++;
    return (gRom08CA6608[high] << 8) | gRom08CA6608[low];
}
