/* Ek istatistik sayaclari (2) — 0x08067374-0x080673A5
 *
 * gSaveBuffer'in +0x7A (u16) ve +0x84 (u8) alanlarina tasma korumali
 * artirim. u16 icin src/world/stat_counters.c'nin desenin aynisi;
 * u8 icin de ayni yapi (bu sefer `lsls #24` sifir-genisletme yeter).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/more_counters_2.c
 */

#include "gba_types.h"

typedef struct SaveCounters {
    u8  pad00[0x7A];
    u16 count7A;                /* +0x7A */
    u8  pad7C[8];
    u8  count84;                /* +0x84 */
} SaveCounters;

extern SaveCounters gSaveBuffer;

/* 0x08067374 */
void BumpCount7A(void)
{
    u16 old;
    int next;

    old = gSaveBuffer.count7A;
    next = old + 1;
    gSaveBuffer.count7A = next;
    if ((u16)next == 0)
        gSaveBuffer.count7A = old;
}

/* 0x08067390 */
void BumpCount84(void)
{
    u8  old;
    int next;

    old = gSaveBuffer.count84;
    next = old + 1;
    gSaveBuffer.count84 = next;
    if ((u8)next == 0)
        gSaveBuffer.count84 = old;
}
