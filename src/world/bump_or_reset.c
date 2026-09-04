/* Doyan sayac — 0x0805AC50-0x0805AC83
 *
 * gRam02000F10.kind == 2 iken iki sorgu zinciri calisiyor; sonuc 0 ise
 * sayaci 254'te DOYURARAK artiriyor, degilse sifirliyor.
 *
 * BYTE-MATCHING. ROM'un `reset`, `increment` ve ortak `store` bloklarini
 * C etiketleriyle acik kurmak iki ayri sayac-adresi yuklemesini ve tek
 * paylasilan `strb`yi aynen uretir.
 *
 * Eslesen kardesi: src/world/counter_saturate.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/bump_or_reset.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define KIND_WANTED   2
#define COUNT_MAX     254

typedef struct Slot {
    u8  pad00[4];
    u32 kind;                   /* +0x04 */
} Slot;

extern u8   gRam02035A9C;

extern u32 GetActiveSlot(void);
extern u32 FUN_08056c80(u32 arg);

/* 0x0805AC50 */
void BumpOrReset(void)
{
    u8 *counter;
    u32 count;

    if (((Slot *)gRam02000F10)->kind != KIND_WANTED)
        goto reset;
    if (FUN_08056c80(GetActiveSlot()) == 0)
        goto increment;

reset:
    counter = &gRam02035A9C;
    count = 0;
    goto store;

increment:
    counter = &gRam02035A9C;
    count = *counter;
    if (count > COUNT_MAX)
        return;
    count++;

store:
    *counter = count;
}
