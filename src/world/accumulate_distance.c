/* Mesafe biriktirme ve tasma sayaci — 0x080672B8-0x080672FB
 *
 * Gelen degerin mutlak degerinin 16 bit sagini biriktiriciye ekliyor;
 * birikim 0x1FFF'i asarsa tasan kismi gSaveBuffer +0x76'daki sayaca
 * aktarip biriktiriciyi maskeliyor. Sayac sararsa eski deger geri
 * yaziliyor (doyurma).
 *
 * ROM sayaci yazdiktan SONRA yeniden OKUYUP karsilastiriyor; bu yuzden
 * karsilastirmadaki okuma volatile gorunumden yapiliyor. Ayni cozum
 * src/world/distance_accum.c'de olculmustu: tum alani volatile yapmak
 * fazla gucludur ve ilk okumayi da bozar, yalnizca ikinci okuma dar
 * tutulmali.
 *
 * SaveBuffer tanimi src/world/copy_flag_byte.c ile BIREBIR AYNI olmali.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/accumulate_distance.c
 */

#include "gba_types.h"

#define ACC_MASK  0x1FFF
#define ACC_SHIFT 13

typedef struct Accum {
    u8  pad00[4];
    u32 value;                  /* +0x04 */
} Accum;

typedef struct SaveBuffer {
    u8 pad00[8];
    u8 byte8;                   /* +0x08 */
    u8 byte9;                   /* +0x09 */
    u8 pad0A[92];
    u16 counter66;              /* +0x66 */
    u8 pad68[14];
    u16 counter76;              /* +0x76 */
} SaveBuffer;

extern Accum      gDistanceAccum;
extern SaveBuffer gSaveBuffer;

/* 0x080672B8 */
void AccumulateDistance(s32 delta)
{
    Accum *accum;
    u32 acc;
    u16 old;

    /* ROM biriktirici tabanini mutlak deger hesabindan ONCE yukluyor
       (ldr r4 en basta). Dogrudan gDistanceAccum yazmak yuklemeyi
       kullanim yerine kaydiriyordu; ayri yerel sirayi sabitliyor. */
    accum = &gDistanceAccum;

    if (delta < 0)
        delta = -delta;

    acc = accum->value + (delta >> 16);
    accum->value = acc;

    if (acc > ACC_MASK) {
        old = gSaveBuffer.counter76;
        gSaveBuffer.counter76 = old + (acc >> ACC_SHIFT);
        acc &= ACC_MASK;
        accum->value = acc;
        if (*(volatile u16 *)&gSaveBuffer.counter76 < old)
            gSaveBuffer.counter76 = old;
    }
}
