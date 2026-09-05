/* Dil secimi ve yerellestirilmis metin okuma — 0x0805E6C4-0x0805E6FF
 *
 * 0x08EC46D4'teki tablo, oyunun METIN ISARETCI TABLOSU: dil basina 618
 * dize, bes dil icin toplam 3090 giris.  gLanguage hangi dilin kullanildigini
 * secer; ucuncu fonksiyon o dilin bloğundan bir dize isaretcisi dondurur.
 *
 * Tablonun ne oldugu DORT BAGIMSIZ YONDEN dogrulandi:
 *   1. Ayarlayici indeksi 0..4'e kirpiyor            -> tam bes deger
 *   2. Okuyucudaki carpan 0x9A8 = 2472 = 618 * 4     -> dil basina 618 dize
 *   3. Tablo boyutu 3090 giris = 618 * 5             -> bes dil
 *   4. Izleme logu: acilis dil ekraninda bes secenek
 *      (gActiveMenuItemCount 0->5, docs/OYUN_AKISI.md)
 * Ayrica her dil bloğunun ilk isaretcisi, o dilin adinin hemen ardina
 * dusuyor (DEUTSCH / FRANCAIS / ITALIANO).  Ayrinti: docs/METIN_HARITASI.md
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/misc/table_lookup.c
 */

#include "gba_types.h"

#define LANGUAGE_MAX     4       /* bes dil: 0..4 */
#define STRINGS_PER_LANG (0x9A8 / 4)   /* 618 */

extern u32 gLanguage;
extern u32 gTextTable[][STRINGS_PER_LANG];   /* 0x08EC46D4, salt okunur */

/* 0x0805E6C4 */
void SetLanguage(u32 index)
{
    if (index <= LANGUAGE_MAX)
        gLanguage = index;
}

/* 0x0805E6D4 */
u32 GetLanguage(void)
{
    return gLanguage;
}

/* 0x0805E6E0 */
u32 GetTextString(u32 index)
{
    return gTextTable[gLanguage][index];
}
