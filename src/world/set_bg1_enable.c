/* DISPCNT bit 9 kurma — 0x0803084C-0x0803085D
 *
 * REG_DISPCNT'e (0x80 << 2) bitini OR'luyor. Kardesi ClearBg1Enable
 * (0x08030860) byte-matching; bu ayri dosyada cunku eslesmiyor ve ayni
 * dosyada birakmak kardesinin bolgesini bozuyor.
 *
 * Denenenler (ucu de 16 bayt): tek yerel; iki yerel (`mask = bit`, kural 37
 * — agbcc kopyayi birlestiriyor); once DISPCNT okuyup maskeyi sonra
 * hesaplamak (sirayi degistirmedi); maskeyi u32 yapmak.
 *
 * Kardesi ClearBg1Enable AYNI adres kalibiyla (0x80 << 19) ILK denemede
 * eslesti; fark yalnizca oradaki islemin AND, buradakinin OR olmasi. Yani
 * engel adres kurulumunda degil, maske degerinin yasam araliginda.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/set_bg1_enable.c
 */

#include "gba_types.h"

#define DISPCNT  (*(vu16 *)(0x80 << 19))
#define BG1_BIT  (0x80 << 2)

/* 0x0803084C */
void SetBg1Enable(void)
{
    DISPCNT = DISPCNT | BG1_BIT;
}
