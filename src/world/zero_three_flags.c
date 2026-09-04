/* Uc bayragi sifirlama — 0x0803376C-0x0803378F
 *
 * gCartFlag baytini, sonra 0x02027320 ve 0x02027310'daki sozleri sifirliyor.
 *
 * ROM IKI ADRESI DE ONCE YUKLUYOR (ldr r2, ldr r1), sonra ters sirada
 * yaziyor. Tek tek yazmak her adresi kendi store'undan hemen once
 * yukletiyordu (10 bayt fark). Ayri taban yerelleri (kural 37) hem yukleme
 * sirasini hem havuz sirasini (0x02027310 once) uretiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/zero_three_flags.c
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
