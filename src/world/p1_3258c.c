/* Iki tablo imlecini ilerletip 16-bit soz uret — 0x0803258C.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x0803258C.json. */
#include "gba_types.h"

extern u32 gRam020272F0;
extern u32 gRam020272FC;
extern u32 gRam020272F4;
extern const u8 gRom08CA6608[];
u32 FUN_0803258c(void)
{
    u32 high,low;
    high = (gRam020272F0 + 3) & 1023;
    gRam020272F0 = high;
    low = (gRam020272FC + 1) & 1023;
    gRam020272FC = low;
    gRam020272F4++;
    return (gRom08CA6608[high] << 8) | gRom08CA6608[low];
}
