/* Alan temizligi — 0x08030CB4-0x08030D0B
 *
 * Bekleyen temizlik bayragi kuruluysa iki VRAM konumuna ucer yarim soz
 * dolduruyor, bir blogu serbest birakiyor ve bayragi temizliyor.
 *
 * HENUZ ESLESMIYOR: 40 komutun 28'i tutuyor, 56 bayt fark. Fark havuz
 * yukleme sirasi ve ROM'un fazladan bir register kopyasi: ROM dolgu
 * degerini r1'e yukleyip `adds r0, r1, #0` ile r0'a kopyaliyor, bizimki
 * dogrudan hedefe yukluyor -- yani bizde bir `ldr` ve bir kopya eksik.
 * Denenenler: blogu dongu oncesi yuklemek (61), dolgudan sonra yuklemek
 * (56, secildi), dolguyu int yapmak (61), satir ici birakmak (63).
 *
 * Ayni kumedeki eslesen uc fonksiyon: src/world/area_flags.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/area_cleanup.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define TILE_FILL      0xF0E8
#define TILE_RUN       3
#define TILE_ROW_LEFT   ((u16 *)0x06009818)
#define TILE_ROW_RIGHT  ((u16 *)0x06009858)

typedef struct Progress {
    u8 pad0000[0x137E];
    u8 pendingCleanup;          /* +0x137E */
} Progress;

extern u32      gRam02026E80;

extern void FUN_08013abc(u32 *block);

/* 0x08030CB4 */
void CleanupAreaTiles(void)
{
    u16 *left;
    u16 *right;
    u32 *block;
    u32 i;
    u16 fill;

    if (((Progress *)gRam02025810)->pendingCleanup == 0)
        return;

    i = 0;
    fill = TILE_FILL;
    block = &gRam02026E80;
    right = TILE_ROW_RIGHT;
    left = TILE_ROW_LEFT;
    do {
        *left = fill;
        *right = fill;
        right++;
        left++;
        i++;
    } while (i <= TILE_RUN - 1);

    FUN_08013abc(block);
    ((Progress *)gRam02025810)->pendingCleanup = 0;
}
