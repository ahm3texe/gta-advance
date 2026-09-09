/* Initialize the menu session — 0x080320E0-0x0803214B
 *
 * Clear gRam02026FD0/gRam02026F30 (rule 37: two base locals, loads first,
 * stores reversed). Exit if gSaveBuffer's active flag +0 is zero. Pass
 * header +0x88 with index 0, then 20-byte entries from +0xA4 with indices
 * 1..count to FUN_080301d8. Copy words +0x10/+0x14 to gClipBounds[0..1]
 * and call FUN_0800d118.
 *
 * MEASURED: use for (i=0; i<count; i++) FUN(&items[i],i+1). agbcc combines
 * i+1 with the increment, producing ROM adds r4,#1 / adds r1,r4. Incrementing
 * i in the body and passing it produces pointer walking (adds r5,#20),
 * 34/51. Read the global directly rather than using local ctx (37).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/ui/init_menu_session.c
 */

#include "gba_types.h"
typedef struct Item { u8 pad00[20]; } Item;
typedef struct MenuCtx { u8 active; u8 pad01[11]; u16 w0C; u16 w0E; u32 w10; u32 w14;
                         u8 pad18[26]; u16 w32; u8 pad34[84];
                         u8 head[0x18]; u8 count; u8 pad; u8 pad2; u8 pad3; Item items[1]; } MenuCtx;
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
