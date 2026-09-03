/* MaybeAdvance — 0x080664F0-0x08066519
 *
 * gVBlankEnabled tam COUNTER_MIN (=2) ise ve sayac <= 1 ise 0 donuyor,
 * degilse 1. Ayni kumedeki uc kardesi src/world/pause_helpers.c'de.
 *
 * HENUZ ESLESMIYOR: 20 komutun 19'u tutuyor, 1 bayt fark. Tek fark
 * karsilastirma yon komutu: ROM `bls`, agbcc her C bicimi icin `bhi`
 * uretiyor. Denenen bes bicim (temel, `&& <=`, `>= 2`, `< 2`, `&& !(>1)`)
 * hepsi 1 verdi -- agbcc bu dalin yonunu ifadeden BAGIMSIZ olarak seciyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/maybe_advance.c
 */

#include "gba_types.h"

#define COUNTER_MIN    2

extern u16  gVBlankEnabled;
extern u16  gRam0200048C;

extern void FUN_080657d8(u32 arg);

/* 0x080664F0 */
u32 MaybeAdvance(void)
{
    FUN_080657d8(0);
    if (gVBlankEnabled == COUNTER_MIN && !(gRam0200048C > 1))
        return 0;

    return 1;
}
