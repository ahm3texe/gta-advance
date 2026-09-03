/* Ek istatistik sayaclari (3) — 0x080673C4-0x080673F5
 *
 * gSaveBuffer'in +0x70 ve +0x80 alanlarina u8 tasma korumali artirim.
 * src/world/more_counters_2.c'deki BumpCount84 deseninin aynisi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/more_counters_3.c
 */

#include "gba_types.h"

typedef struct SaveCounters {
    u8  pad00[0x70];
    u8  count70;                /* +0x70 */
    u8  pad71[0x0F];
    u8  count80;                /* +0x80 */
} SaveCounters;

extern SaveCounters gSaveBuffer;

/* 0x080673C4 */
void BumpCount70(void)
{
    u8  old;
    int next;

    old = gSaveBuffer.count70;
    next = old + 1;
    gSaveBuffer.count70 = next;
    if ((u8)next == 0)
        gSaveBuffer.count70 = old;
}

/* 0x080673E0 */
void BumpCount80(void)
{
    u8  old;
    int next;

    old = gSaveBuffer.count80;
    next = old + 1;
    gSaveBuffer.count80 = next;
    if ((u8)next == 0)
        gSaveBuffer.count80 = old;
}
