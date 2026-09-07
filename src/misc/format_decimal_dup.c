/* Ondalik basamak yazicinin IKINCI kopyasi — 0x080674B0-0x08067523
 *
 * ROM'da bu fonksiyon IKI KEZ var. 0x080672FC ile bu adres arasindaki
 * 116 baytin md5'i BIREBIR AYNI (dogrulandi); yani ayni kaynak iki ayri
 * derleme birimine girmis ve baglayici tekillestirmemis.
 *
 * Govde src/misc/format_decimal.c ile ayni; yalnizca fonksiyon adi
 * farkli. Ikisi ayri ayri kayitli olmali cunku ROM'da ayri adreslerde
 * duruyorlar.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/misc/format_decimal_dup.c
 */

#include "gba_types.h"

#define POWER_COUNT 10

extern const u32 gPowersOfTen[];

/* 0x080674B0 */
void FormatDecimalDup(u32 value, u8 *out)
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
