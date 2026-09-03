/* Doyan sayac — 0x0805AC50-0x0805AC83
 *
 * gRam02000F10.kind == 2 iken iki sorgu zinciri calisiyor; sonuc 0 ise
 * sayaci 254'te DOYURARAK artiriyor, degilse sifirliyor.
 *
 * HENUZ ESLESMIYOR: 41 bayt fark. ROM iki dalin `strb` komutunu
 * PAYLASTIRIYOR ama `ldr r1, =sayac` yuklemesini her iki dalda AYRI
 * yapiyor. Denenenler: iki ayri store (41), ayri `value` yereli (41),
 * tek cikisli `if/else` + son store (44). Hicbiri ROM'un "paylasilan
 * store, ayri taban yuklemesi" desenini uretmedi.
 *
 * Eslesen kardesi: src/world/counter_saturate.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/bump_or_reset.c
 */

#include "gba_types.h"

#define KIND_WANTED   2
#define COUNT_MAX     254

typedef struct Slot {
    u8  pad00[4];
    u32 kind;                   /* +0x04 */
} Slot;

extern Slot gRam02000F10;
extern u8   gRam02035A9C;

extern u32 GetActiveSlot(void);
extern u32 FUN_08056c80(u32 arg);

/* 0x0805AC50 */
void BumpOrReset(void)
{
    u8 count;

    if (gRam02000F10.kind == KIND_WANTED) {
        if (FUN_08056c80(GetActiveSlot()) == 0) {
            count = gRam02035A9C;
            if (count > COUNT_MAX)
                return;
            gRam02035A9C = count + 1;
            return;
        }
    }

    gRam02035A9C = 0;
}
