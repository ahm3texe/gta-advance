/* Sirali sayac artirimi — 0x08066C94-0x08066D53
 *
 * Kayit tamponunun +0x7E'sindeki tek u16 icinde uc adet 5 bitlik sayac
 * var. Hangi kaydin etkin oldugunu GetRecordIndex soyluyor ve o kaydin
 * sayaci bir artirilip 20'de doyuruluyor.
 *
 * Ucu de AYNI kaynak kalibindan cikiyor; agbcc alanin bayt sinirini
 * asmasina gore farkli komut uretiyor:
 *   bit 1-5   -> tek bayt  (ldrb/strb +0x7E)
 *   bit 6-10  -> yarim soz (ldrh/strh +0x7E, siniri asiyor)
 *   bit 11-15 -> tek bayt  (ldrb/strb +0x7F)
 * Yani ucunu ayri ayri yazmaya gerek yok, bitfield bildirimi yeterli.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/bump_rank_counter.c
 */

#include "gba_types.h"

#define COUNTER_CAP 20

typedef struct RankCounters {
    u8  pad00[0x7E];
    u16 spare  : 1;             /* bit 0 */
    u16 countA : 5;             /* bit 1-5 */
    u16 countB : 5;             /* bit 6-10 */
    u16 countC : 5;             /* bit 11-15 */
} RankCounters;

extern RankCounters gSaveBuffer;
extern s32 GetRecordIndex(void);

/* 0x08066C94 */
void BumpRankCounter(void)
{
    switch (GetRecordIndex()) {
    case 0:
        gSaveBuffer.countA++;
        if (gSaveBuffer.countA > COUNTER_CAP)
            gSaveBuffer.countA = COUNTER_CAP;
        break;
    case 1:
        gSaveBuffer.countB++;
        if (gSaveBuffer.countB > COUNTER_CAP)
            gSaveBuffer.countB = COUNTER_CAP;
        break;
    case 2:
        gSaveBuffer.countC++;
        if (gSaveBuffer.countC > COUNTER_CAP)
            gSaveBuffer.countC = COUNTER_CAP;
        break;
    }
}
