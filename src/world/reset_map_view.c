/* Harita gorunumunu sifirlama — 0x08030F90-0x08031003
 *
 * ClearMapWindow'dan sonra gRam02025810 blogunun +0x1358 ve +0x1368
 * sozlerini sifirlar; +0x137C bayti kuruluysa gRam02027280 nesnesini
 * ReleaseObject ile birakip bayragi siler; +0x1364 = 1; FUN_08029918;
 * +0x136C sifir degilse +0x112C degeriyle FUN_0802b01c ve +0x1130 = 0.
 * Blok icin release_slot.c'deki bayt-uzaklik yazimi kullanildi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/reset_map_view.c
 */

#include "gba_types.h"
#include "ram_symbols.h"
extern u8   gRam02027280[];
extern void ClearMapWindow(void);
extern void ReleaseObject(u8 *obj);
extern void FUN_08029918(void);
extern void FUN_0802b01c(u32 v);
void ResetMapView(void)
{
    u8 *base; u8 *flag;
    ClearMapWindow();
    base = gRam02025810;
    *(u32 *)(base + 0x1358) = 0;
    *(u32 *)(base + 0x1368) = 0;
    flag = base + 0x137C;
    if (*flag != 0) {
        ReleaseObject(gRam02027280);
        *flag = 0;
    }
    *(u32 *)(base + 0x1364) = 1;
    FUN_08029918();
    if (*(u32 *)(base + 0x136C) != 0) {
        FUN_0802b01c(*(u32 *)(base + 0x112C));
        *(u32 *)(base + 0x1130) = 0;
    }
}
