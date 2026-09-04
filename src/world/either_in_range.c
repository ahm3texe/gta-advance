/* Iki alandan biri aralikta mi — 0x080195F4-0x0801961F
 *
 * 12-13 ya da 15-16 araliklarini sinayan iki kapi. Birinci alan ISARETLI
 * karsilastiriliyor (blt/ble/bgt/bge), ikincisi ISARETSIZ (bcc/bls/bhi).
 *
 * ALAN u16 OLMALI, s16 DEGIL. ROM `ldrh` ile ISARETSIZ yukluyor ama
 * karsilastirmalar isaretli: bu, C'nin dogal davranisi -- u16 alan
 * karsilastirmada `int`e yukseltilir. s16 yazmak `ldrsh` uretiyordu
 * (2 bayt fazla). Ikinci alan u32 oldugu icin karsilastirmalari
 * isaretsiz kaliyor.
 *
 * HENUZ ESLESMIYOR: 8/44. Boyut DOGRU; kalan farklarin HEPSI ayni sinif --
 * karsilastirma sabitinin kanonikleştirilmesi:
 *     bizim: cmp #11 / ble        ROM: cmp #12 / blt
 *     bizim: cmp #14 / bgt        ROM: cmp #15 / bge
 * Anlamca ayni, bicimce farkli. agbcc `< 12`yi `<= 11`e ceviriyor ve
 * sabit bir eksik yaziliyor. Sekiz yerde birden.
 *
 * Denenenler: erken cikisli `goto` (8, en iyi); ic ice pozitif kosul
 * (15, DAHA KOTU -- ilk yarim zaten oyleydi ve ayni kanonikleştirme
 * cikiyordu). Sonraki fikir: kural 30 yonunde acik esitlik zinciri
 * (`x == 12 || x == 13 || ...`) denemek.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/either_in_range.c
 */

#include "gba_types.h"

typedef struct Entry {
    u8  pad00[6];
    u16 kind;                   /* +0x06 */
    u8  pad08[32];
    u32 alt;                    /* +0x28 (isaretsiz) */
} Entry;

/* 0x080195F4 */
u32 EitherInRange(Entry *entry)
{
    s32 kind;
    u32 alt;

    kind = entry->kind;
    if (kind >= 12) {
        if (kind <= 13)
            goto yes;
        if (kind <= 16) {
            if (kind >= 15)
                goto yes;
        }
    }

    alt = entry->alt;
    if (alt < 12)
        goto no;
    if (alt <= 13)
        goto yes;
    if (alt > 16)
        goto no;
    if (alt < 15)
        goto no;

yes:
    return 1;
no:
    return 0;
}
