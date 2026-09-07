/* Yuvanin tuttugu nesneyi birakma — 0x08031658-0x08031683
 *
 * ReleaseSlotSub (src/video/blit_strip_plain.c, 0x08031684) ile KOMUT
 * KOMUT ayni; iki fark var: bayrak eleman ici +0xB0 (blok basi +0xEC)
 * ve birakilan alan eleman ici +0x1C (blok basi +0x58).
 * tools/find_twins.py %90.9 benzerlikle isaret etti.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/release_slot_held.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define SLOT_STRIDE  180            /* 0xB4 */
#define OFF_HELD     0x58           /* blok basi; eleman ici +0x1C */
#define OFF_FLAG     0xEC           /* blok basi; eleman ici +0xB0 */

extern void FUN_08013abc(u8 *sub);

/* 0x08031658 */
void ReleaseSlotHeld(u32 index)
{
    u8 *base;
    u8 *flag;
    u8 *heldBase;
    u32 scaled;

    base = gRam02025810;
    scaled = index * SLOT_STRIDE;
    flag = base + scaled + OFF_FLAG;
    if (*flag == 0)
        return;

    heldBase = base + OFF_HELD;
    FUN_08013abc(heldBase + scaled);
    *flag = 0;
}
