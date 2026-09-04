/* Yuva serbest birakma — 0x080308AC-0x080308E3
 *
 * gRam02025810 blogunda +0x4C'de 24 girisli 180 baytlik yuva dizisi;
 * her giriste +0 nesne isaretcisi, +4 (blokta +0x50) yardimci alan.
 * Serbest birakirken nesnenin +0x0C bayragindan 0x02000000 temizleniyor.
 *
 * BYTE-MATCHING. `scaled = index * 180` tek ifadesi ile `heldBase` ve
 * `extraBase`i ayri yerellerde kurmak ROM'un bir kez hesaplanan r3 ofsetini
 * iki tabana ekleme desenini korur.
 *
 * Eslesen kardesi: src/world/slot_release.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/release_slot.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define SLOT_MAX        23
#define SLOT_STRIDE     180
#define FLAG_CLEAR      0xFDFFFFFF      /* ~0x02000000 */

typedef struct Held {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} Held;

/* 0x080308AC */
u32 ReleaseSlot(u32 index)
{
    u8    *base;
    u8    *heldBase;
    u8    *extraBase;
    Held **heldSlot;
    u32    scaled;
    u32   *extraSlot;
    Held  *held;

    if (index <= SLOT_MAX) {
        base = gRam02025810;
        scaled = index * SLOT_STRIDE;
        heldBase = base + 0x4C;
        heldSlot = (Held **)(heldBase + scaled);
        held = *heldSlot;
        if (held != 0)
            held->flags &= FLAG_CLEAR;

        extraBase = base + 0x50;
        extraSlot = (u32 *)(extraBase + scaled);
        *extraSlot = 0;
        *heldSlot = 0;
    }

    return 1;
}
