/* Ondalik basamak yazici — 0x080672FC-0x0806736F
 *
 * Sayiyi bastaki sifirlar olmadan ASCII basamaklara ceviriyor. Bolme
 * yok: 0x08FD17FC'deki 10^0..10^9 tablosundan tekrarli cikarma yapiyor
 * (agbcc'de `/` __divsi3 uretirdi, ROM'da o cagri yok).
 *
 * Once degeri kapsayan en yuksek kuvvet bulunuyor, sonra o basamaktan
 * asagi dogru her kuvvet icin cikarma sayisi '0' uzerine ekleniyor.
 * Basamak u8: ROM her artirimdan sonra 24 bit kaydirmayla kirpiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/misc/format_decimal.c
 */

#include "gba_types.h"

#define POWER_COUNT 10

extern const u32 gPowersOfTen[];

/* 0x080672FC */
void FormatDecimal(u32 value, u8 *out)
{
    s32 i;
    u8  digit;

    for (i = POWER_COUNT - 1; i >= 0; i--) {
        if (value >= gPowersOfTen[i])
            break;
    }
    if (i < 0)
        i = 0;

    do {
        digit = '0';
        while (value >= gPowersOfTen[i]) {
            value -= gPowersOfTen[i];
            digit++;
        }
        *out = digit;
        out++;
        i--;
    } while (i >= 0);

    *out = 0;
}
