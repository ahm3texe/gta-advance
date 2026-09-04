/* Mesafe biriktirici — 0x08067274-0x080672B7
 *
 * Her cagride |delta| >> 16 ekliyor; biriken deger 0x1FFF'i asinca tasan
 * kismi (>> 13) kayittaki sayaca aktarip biriktiriciyi maskeliyor.
 * Sondaki karsilastirma tasma korumasi.
 *
 * BYTE-MATCHING. Ayrı `accum` isaretcisi tabani isaret duzeltmesinden once
 * r4'e yukletir. Yalniz tasma karsilastirmasindaki dar volatile gorunum,
 * yazilan distance alanini ROM'daki gibi bellekten yeniden okutur.
 *
 * Ayni kumedeki eslesen uc sayac: src/world/stat_counters.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/distance_accum.c
 */

#include "gba_types.h"

#define ACCUM_MASK   0x1FFF
#define ACCUM_SHIFT  13
#define DELTA_SHIFT  16

typedef struct SaveCounters {
    u8  pad00[0x74];
    u16 distance;               /* +0x74 */
} SaveCounters;

extern SaveCounters gSaveBuffer;
extern u32 gDistanceAccum;

/* 0x08067274 */
void AddDistance(int delta)
{
    u32 *accum;
    u32 total;
    u16 old;

    accum = &gDistanceAccum;
    if (delta < 0)
        delta = -delta;

    total = *accum + (delta >> DELTA_SHIFT);
    *accum = total;

    if (total > ACCUM_MASK) {
        old = gSaveBuffer.distance;
        gSaveBuffer.distance = old + (total >> ACCUM_SHIFT);
        *accum = total & ACCUM_MASK;
        if (*(volatile u16 *)&gSaveBuffer.distance < old)
            gSaveBuffer.distance = old;
    }
}
