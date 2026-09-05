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
 * ESLESTI (44/44) -- bkz. fonksiyon ustundeki MEKANIZMA notu.
 */

#include "gba_types.h"

typedef struct Entry {
    u8  pad00[6];
    u16 kind;                   /* +0x06 */
    u8  pad08[32];
    u32 alt;                    /* +0x28 (isaretsiz) */
} Entry;

/* 0x080195F4 */
/* MEKANIZMA (permuter + elle, 2026-09-05): karsilastirma sabiti bir
 * DEGISKENE alininca agbcc `x < 15`i `x <= 14`e KANONIKLESTIREMIYOR ve
 * ROM'un `cmp #15 / bcc` biciminin aynisini uretiyor.  Literal yazimin
 * hicbir cesidi (< 15, <= 14, > 14 ...) bunu vermiyordu: hepsi ayni
 * kanonik forma cokuyor.  Uc sinir da boyle tasindi.  docs/COMPILER.md
 * kural 44. */
u32 EitherInRange(Entry *entry)
{
    s32 lowBound;
    s32 highBound;
    s32 kind;
    u32 alt;
    u32 altHigh;

    kind = entry->kind;
    lowBound = 12;
    if (kind >= lowBound) {
        if (kind <= 13)
            goto yes;
        if (kind <= 16) {
            if (kind >= (highBound = 15))
                goto yes;
        }
    }
    alt = entry->alt;
    if (alt < lowBound)
        goto no;
    if (alt <= 13)
        goto yes;
    if (alt > 16)
        goto no;
    altHigh = 15;
    if (alt < altHigh)
        goto no;
yes:
    return 1;
no:
    return 0;
}
