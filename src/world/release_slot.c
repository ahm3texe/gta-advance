/* Yuva serbest birakma — 0x080308AC-0x080308E3
 *
 * gRam02025810 blogunda +0x4C'de 24 girisli 180 baytlik yuva dizisi;
 * her giriste +0 nesne isaretcisi, +4 (blokta +0x50) yardimci alan.
 * Serbest birakirken nesnenin +0x0C bayragindan 0x02000000 temizleniyor.
 *
 * HENUZ ESLESMIYOR: 47 bayt fark. ROM `index * 180`i BIR KEZ hesaplayip
 * iki farkli tabana (base+76 ve base+80) ekliyor; benim struct bicimim
 * tek isaretci uzerinden iki alan okuyor. Denenenler: tek SlotPair
 * isaretcisi (47), acik bayt ofsetleri (53), iki ayri SlotPair yereli
 * (47). Hicbiri ROM'un iki-taban desenini uretmedi.
 *
 * Eslesen kardesi: src/world/slot_release.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/release_slot.c
 */

#include "gba_types.h"

#define SLOT_MAX        23
#define SLOT_STRIDE     180
#define FLAG_CLEAR      0xFDFFFFFF      /* ~0x02000000 */

typedef struct Held {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} Held;

typedef struct SlotPair {
    Held *held;
    u32   extra;
    u8    pad08[SLOT_STRIDE - 8];
} SlotPair;

typedef struct Pack {
    u8       pad0000[0x4C];
    SlotPair slots[SLOT_MAX + 1];
} Pack;

extern Pack gRam02025810;

/* 0x080308AC */
u32 ReleaseSlot(u32 index)
{
    SlotPair *a;
    SlotPair *b;
    Held     *held;

    if (index <= SLOT_MAX) {
        a = &gRam02025810.slots[index];
        held = a->held;
        if (held != 0)
            held->flags &= FLAG_CLEAR;

        b = &gRam02025810.slots[index];
        b->extra = 0;
        a->held = 0;
    }

    return 1;
}

