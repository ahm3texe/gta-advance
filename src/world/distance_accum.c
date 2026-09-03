/* Mesafe biriktirici — 0x08067274-0x080672B7
 *
 * Her cagride |delta| >> 16 ekliyor; biriken deger 0x1FFF'i asinca tasan
 * kismi (>> 13) kayittaki sayaca aktarip biriktiriciyi maskeliyor.
 * Sondaki karsilastirma tasma korumasi.
 *
 * HENUZ ESLESMIYOR: 36 komutun 27'si tutuyor, 38 bayt fark. Iki neden:
 *   - ROM biriktirici tabanini fonksiyonun basinda, isaret duzeltmesinden
 *     ONCE bir kez yukluyor; bizimki sonra yukleyip bir kez daha yukluyor.
 *   - ROM saklanan mesafeyi karsilastirma icin bellekten YENIDEN OKUYOR
 *     (`ldrh`); bizimki degeri register'da tutup `lsls/lsrs` ile
 *     genisletiyor. Ikisi de dogru, ROM'unki farkli secim.
 * Denenenler: ayri `next` yereli (44), karsilastirmayi ters cevirmek (38),
 * biriktiriciyi ayri deyimde okumak (43). Hicbiri temeli gecmedi.
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
    u32 total;
    u16 old;

    if (delta < 0)
        delta = -delta;

    total = gDistanceAccum + (delta >> DELTA_SHIFT);
    gDistanceAccum = total;

    if (total > ACCUM_MASK) {
        old = gSaveBuffer.distance;
        gSaveBuffer.distance = old + (total >> ACCUM_SHIFT);
        gDistanceAccum = total & ACCUM_MASK;
        if (gSaveBuffer.distance < old)
            gSaveBuffer.distance = old;
    }
}
