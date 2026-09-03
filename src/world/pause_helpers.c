/* Duraklatma yardimcilari — 0x080664A4-0x080664EF
 *
 * Dort kucuk fonksiyon. gVBlankEnabled (u16) durum kelimesi;
 * gGameState[12] duraklatma bayragi; 0x02036328 ek bayrak.
 *
 * Ayni kumedeki dorduncu fonksiyon (MaybeAdvance, 0x080664F0) 1 bayt
 * farkli ve ayri dosyada: src/world/maybe_advance.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/pause_helpers.c
 */

#include "gba_types.h"

#define BASE_VALUE     (232 << 1)     /* 464, ROM `movs #232; lsls #1` */
#define STATE_ACTIVE   3

extern u16  gVBlankEnabled;
extern u8   gGameState[];
extern u8   gRam02036328;

extern void FUN_080657d8(u32 arg);

/* 0x080664A4 */
u32 GetBaseValue(void)
{
    return BASE_VALUE;
}

/* 0x080664AC */
void MaybeReset(void)
{
    if (gVBlankEnabled != 0)
        FUN_080657d8(2);
}

/* 0x080664C4 */
u32 IsSessionActive(void)
{
    if (gGameState[12] == 0)
        return 0;
    if (gVBlankEnabled != STATE_ACTIVE) {
        if (gRam02036328 == 0)
            return 0;
    }

    return 1;
}
