/* Menu dongusu — 0x08001A00-0x08001DBF
 *
 * Menu hiyerarsisini kurar, girisi isler ve secili menuyu her karede
 * yeniden cizer. 0x08001C24'teki `mov pc, r0` atlama tablosu, eylem
 * fonksiyonunun donus degerine gore dallanan bes vakali switch'ten gelir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/ui/menu_loop.c
 */

#include "gba_io.h"

/* Thumb dolayli cagri yardimcisi (libgcc `_call_via_rN`): `bx r4`. */

/* data/ram_map.csv'de olmayan RAM adresleri. Sabit cast yazilirsa agbcc
 * 0x0300009C'yi 0x03000098+4 diye katliyor (docs/COMPILER.md kural 1);
 * ayri sembol olunca ROM'daki gibi ayri literal cikiyor. */

#define BLEND_Y          (*(u16 *)0x04000054)
#define PALETTE_RAM      ((void *)0x05000000)
#define VRAM             ((void *)0x06000000)

#define DMA_SAVE_PALETTE 0x80000100
#define DMA_COPY_PALETTE 0x80000010
#define DMA_CLEAR_VRAM   0x81004B00

#define MENU_TILES_A     ((const void *)0x08831880)
#define MENU_TILES_B     ((const void *)0x0883F880)
#define MENU_PALETTE_0   ((const void *)0x08831680)
#define MENU_PALETTE_1   ((const void *)0x08EC7A44)
#define PALETTE_DEST_0   ((void *)0x05000140)
#define PALETTE_DEST_1   ((void *)0x05000180)

#define BLEND_ALL        0xFF
#define BLEND_Y_MAX      31
#define VRAM_FILL_VALUE  0x9090
#define MENU_DISPCNT     0x0101
#define MENU_TILE_COUNT  30
#define MENU_TILE_WIDTH  160

#define MENU_VISIBLE_MAX 8
#define SOUND_MENU_MOVE  0x105

#define KEY_A            0x0001
#define KEY_B            0x0002
#define KEY_UP           0x0040
#define KEY_DOWN         0x0080

#define gKeys            (*(u16 *)0x02000D0C)
#define gUnk02001200     (*(u8 *)0x02001200)

typedef struct Menu Menu;

typedef struct MenuItem {
    u32   type;             /* +0  0: cikis, 1: alt menu, 2: eylem */
    u32   label;            /* +4 */
    int   value;            /* +8 */
    u32  *conditionFlags;   /* +12 */
    u32   requiredBits;     /* +16 */
    Menu *submenu;          /* +20 */
    u32 (*action)(Menu *menu, int selected, u32 param, int value); /* +24 */
    u32   param;            /* +28 */
} MenuItem;

struct Menu {
    u32   centered;         /* +0 */
    Menu *parent;           /* +4 */
    u32   unk8;             /* +8 */
    u32   unk12;            /* +12 */
    int   kind;             /* +16 baslangic seciminin nasil kuruldugu */
    int   itemCount;        /* +20 */
    MenuItem items[1];      /* +24 */
};

#define ROOT_MENU ((Menu *)0x0832FB60)

extern u8   gActiveMenuItemCount;
extern MenuItem *gActiveMenuItems[20];
extern u8   gMenuPaletteSource[];
extern u8   gUnk02010C60[];
extern u32  gRam03000098;
extern u32  gRam0300009C;
extern u16  gRam02000498;
extern u32  gRam030000A0;

extern Menu *BuildActiveMenuItems(Menu *menu);
extern void  DrawMenuItems(Menu *menu, int selected, int firstItem);
extern void  SetTableIndex(int index);

extern void  FUN_080082c4(void);
extern void  FUN_08012690(void);
extern void  FUN_0803378c(int a, int b);
extern void  FUN_08004280(u32 a, u32 b);
extern void  FUN_0806430c(void *dest, u32 tileCount, const void *tilesA,
                          const void *tilesB, u32 width, u32 flags);
extern void  FUN_08063ca4(void);
extern void  FUN_0800ab60(void);
extern void  FUN_08005fc4(int id);
extern u32   GetRecordIndex(void);
extern void  VBlankIntrWait(void);
extern void  FUN_08063cf0(void);

/* 0x08001A00 */
void RunMenuLoop(void)
{
    volatile u16 fill;
    u16 ime;
    u8 *saved;
    Menu *menu;
    MenuItem *item;
    int sel;
    int scrollTop;
    int savedSel;
    int redraw;
    int running;

    SetTableIndex(0);
    FUN_080082c4();

    REG_BLDCNT = BLEND_ALL;
    REG_BLDALPHA = 0;
    BLEND_Y = BLEND_Y_MAX;

    saved = gMenuPaletteSource;
    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = PALETTE_RAM;
    REG_DMA3.dst = saved;
    REG_DMA3.control = DMA_SAVE_PALETTE;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = MENU_PALETTE_1;
    REG_DMA3.dst = PALETTE_DEST_0;
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = MENU_PALETTE_0;
    REG_DMA3.dst = PALETTE_DEST_1;
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = VRAM_FILL_VALUE;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = VRAM;
    REG_DMA3.control = DMA_CLEAR_VRAM;
    REG_DMA3.control;
    REG_IME = ime;

    FUN_08012690();
    FUN_0803378c(1, 1);

    menu = BuildActiveMenuItems(ROOT_MENU);
    FUN_08004280(108, 0);

    FUN_0806430c(VRAM, MENU_TILE_COUNT, MENU_TILES_A, MENU_TILES_B,
                 MENU_TILE_WIDTH, 0);

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = MENU_PALETTE_0;
    REG_DMA3.dst = PALETTE_DEST_0;
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = MENU_PALETTE_1;
    REG_DMA3.dst = PALETTE_DEST_1;
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
    REG_IME = ime;

    DrawMenuItems(menu, 0, 0);
    FUN_08063ca4();

    running = 1;
    sel = 0;
    scrollTop = 0;
    redraw = 0;
    savedSel = 0;

    do {
        FUN_0800ab60();
        REG_DISPCNT = MENU_DISPCNT;

        if ((gKeys & KEY_A) != 0) {
            FUN_08005fc4(SOUND_MENU_MOVE);
            item = gActiveMenuItems[sel];
            switch (item->type) {
            case 0:
                running = 0;
                break;
            case 1:
                savedSel = sel;
                menu = BuildActiveMenuItems(menu->items[sel].submenu);
                switch (menu->kind) {
                case 1:
                    sel = gUnk02001200 != 0;
                    break;
                case 2:
                    sel = gUnk02010C60[26] != 0;
                    break;
                case 3:
                    sel = 3 & ~gUnk02010C60[1];
                    break;
                case 4:
                    sel = gUnk02010C60[6];
                    if (gUnk02010C60[6] > 3) {
                        sel = 0;
                        gUnk02010C60[6] = 0;
                    }
                    break;
                default:
                    sel = 0;
                    break;
                }
                scrollTop = 0;
                redraw = 1;
                break;
            case 2:
                REG_DISPCNT = MENU_DISPCNT;
                switch (item->action(menu, sel, item->param, item->value)) {
                case 0:
                    redraw = 1;
                    break;
                case 1:
                    if (menu->parent != 0) {
                        menu = BuildActiveMenuItems(menu->parent);
                        sel = savedSel;
                        scrollTop = 0;
                        redraw = 1;
                    } else {
                        running = 0;
                    }
                    break;
                case 2:
                    running = 0;
                    break;
                case 3:
                    menu = BuildActiveMenuItems(ROOT_MENU);
                    sel = 0;
                    scrollTop = 0;
                    redraw = 1;
                    break;
                case 4:
                    menu = BuildActiveMenuItems(menu);
                    sel = 0;
                    scrollTop = 0;
                    redraw = 1;
                    break;
                }
                break;
            }
        }

        if ((gKeys & KEY_B) != 0) {
            FUN_08005fc4(SOUND_MENU_MOVE);
            if (menu->parent != 0) {
                menu = BuildActiveMenuItems(menu->parent);
                sel = savedSel;
                scrollTop = 0;
                redraw = 1;
            } else {
                running = 0;
            }
        }

        if ((gKeys & KEY_UP) != 0) {
            FUN_08005fc4(SOUND_MENU_MOVE);
            if (sel > 0) {
                sel--;
                SetTableIndex(sel);
                if (sel < scrollTop) {
                    scrollTop--;
                    FUN_08004280(GetRecordIndex() + 108, 0);
                } else {
                    FUN_08004280(GetRecordIndex() + 108, 0);
                }
                redraw = 1;
            }
        } else if ((gKeys & KEY_DOWN) != 0) {
            FUN_08005fc4(SOUND_MENU_MOVE);
            if (sel < gActiveMenuItemCount - 1) {
                sel++;
                SetTableIndex(sel);
                if (sel > scrollTop + (MENU_VISIBLE_MAX - 1)) {
                    scrollTop++;
                    FUN_08004280(GetRecordIndex() + 108, 0);
                } else {
                    FUN_08004280(GetRecordIndex() + 108, 0);
                }
                redraw = 1;
            }
        }

        if (redraw != 0) {
            REG_DISPCNT = MENU_DISPCNT;
            DrawMenuItems(menu, sel, scrollTop);
            redraw = 0;
        }

        VBlankIntrWait();
    } while (running != 0);

    REG_DISPCNT = MENU_DISPCNT;
    gRam030000A0 = gRam02000498 = gKeys = gRam0300009C = gRam03000098 = 0;
    FUN_08063cf0();
}
