/* Menu oturumunu baslatma — 0x080320E0-0x0803214B
 *
 * gRam02026FD0 ve gRam02026F30 sifirlanir (kural 37: iki taban yereli,
 * yuklemeler once, yazimlar ters sirada). gSaveBuffer'in +0 etkin bayti
 * sifirsa cikilir; +0x88 basligi 0 indisiyle, sonra +0xA4'ten baslayan 20
 * baytlik girisler 1..count indisleriyle FUN_080301d8'e verilir; +0x10 ve
 * +0x14 sozleri gClipBounds[0..1]'e kopyalanip FUN_0800d118 cagrilir.
 *
 * OLCULEN: dongu `for (i = 0; i < count; i++) FUN(&items[i], i + 1)`
 * yazilmali -- agbcc `i + 1`i dongu artirimiyla birlestirip ROM'un
 * `adds r4,#1 / adds r1,r4` ciftini uretiyor. `i++` govde icinde yazilip
 * sonucun gecirilmesi isaretci yurutmesine (`adds r5,#20`) donusuyor
 * (34/51); yerel `ctx` isaretcisi yerine global dogrudan okunmali (37).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/ui/init_menu_session.c
 */

#include "gba_types.h"
typedef struct Item { u8 pad00[20]; } Item;
typedef struct MenuCtx { u8 active; u8 pad01[15]; u32 w10; u32 w14; u8 pad18[0x70]; u8 head[0x18]; u8 count; u8 pad; u8 pad2; u8 pad3; Item items[1]; } MenuCtx;
extern u32 gRam02026F30;
extern u32 gRam02026FD0;
extern MenuCtx gSaveBuffer;
extern u32 gClipBounds[];

extern void FUN_080301d8(void *item, u32 index);
extern void FUN_0800d118(void);
void InitMenuSession(void)
{
    u32 *low; u32 *high; s32 i; Item *item;
    low = &gRam02026F30;
    high = &gRam02026FD0;
    *high = 0;
    *low = 0;
    if (gSaveBuffer.active == 0)
        return;
    FUN_080301d8(gSaveBuffer.head, 0);
    for (i = 0; i < gSaveBuffer.count; i++)
        FUN_080301d8(&gSaveBuffer.items[i], i + 1);
    gClipBounds[0] = gSaveBuffer.w10;
    gClipBounds[1] = gSaveBuffer.w14;
    FUN_0800d118();
}
