/* Reset the map view — 0x08030F90-0x08031003
 *
 * After ClearMapWindow, clear words +0x1358/+0x1368 in gRam02025810. If byte
 * +0x137C is set, release gRam02027280 through ReleaseObject and clear the
 * flag. Set +0x1364 = 1 and call FUN_08029918. If +0x136C is nonzero, call
 * FUN_0802b01c with +0x112C and set +0x1130 = 0. Uses the byte-offset block
 * access form from release_slot.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/reset_map_view.c
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
