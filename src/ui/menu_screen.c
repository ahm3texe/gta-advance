/* Menu ekrani — 0x08001458-0x080019DD
 *
 * Duraklatma/menu ekranini kurar ve kapanana kadar surer. RunMenuLoop
 * (src/ui/menu_loop.c) ile ayni cizim dongusunu paylasir; farki, hangi
 * menu agacinin acilacagini secen bir onsoz ve gorev listesi acikken
 * gosterilen uyari mesajlari.
 *
 * Iki atlama tablosu var:
 *   0x080014CC  41 giris (case 17..57), yalnizca iki hedef
 *   0x08001874   5 giris, eylem fonksiyonunun donus degeri
 *
 * HENUZ ESLESMIYOR ama BOYUT TUTTU: 1448/1448, bayt farki 502, hizali
 * komut 582/679.  Bastan 0x080015D8'e kadarki onsoz -- gorev kontrolu, 41
 * girisli atlama tablosu, uyari mesajlari -- ROM ile AYNI BAYT.
 *
 * KURAL 45 BURAYA DA UYDU (docs/COMPILER.md).  Onceki surumde alti IME
 * sakla/geri-yaz cifti TEK bir `ime` yerelini paylasiyordu; cikti 1456 bayt
 * (8 fazla), fark 949, hizali komut 494.  Birinci, ikinci, dorduncu ve
 * altinci cifte KENDI blok-yerel `savedIme` degiskeni verilince boyut
 * tuttu ve fark 502'ye indi.
 *
 * ALT KUME ONEMLI, hepsini kapsamak KOTU: alti cifti de blok-yerel yapmak
 * 1444 bayt / fark 643 veriyor, ilk dordu 1444 / 643.  Yalnizca (1,2,4,6)
 * dogru.  Bu, dagitimin blok basina degil, ROM'un canli deger duzenine
 * gore ayarlandigi anlamina geliyor.
 *
 * IKINCI DUZELTME -- KATLANMIS HAVUZ SABITI.  `((int *)gRam02025810)[5]`
 * yazimi taban+0x14'u TEK havuz sabitine (0x02025824) katliyor ve
 * `ldr r1,[r0,#0]` uretiyordu; ROM tabani duz yukleyip `ldr r1,[r0,#20]`
 * yapiyor.  Yapi uyesi bicimine gecirmek fark 502 -> 499, hizali komut
 * 582 -> 584, ayrim blogu 57 -> 55.  Uc yazim denendi (yerel int*, yerel
 * struct*, dogrudan uye), UCU DE ayni sonucu verdi.
 *
 * Kalan fark hala yazmac dagitiminda: ROM saved->r2, ime->r1, sifir->r3
 * tutuyor.  Sonraki sapmalar havuz KONUMU farki (bizimki 4 bayt ileriyi
 * okuyor), yani havuz sirasi kaymis; toplam boyut ayni oldugu icin bir
 * giris yer degistirmis olmali.
 *
 * Denenenler (hicbiri ilerletmedi): sifiri yerele alip yazmacta sabitlemek
 * (`u16 off = 0; REG_IME = off;`) -- bayt farki 949'dan 924'e dusuyor ama
 * KOMUT HIZALAMASI 494'ten 473'e geriliyor ve boyut 1456'da kaliyor, yani
 * kazanc degil kayma; iki olcut celisince yapisal olana bakildi.  Ayrica:
 * case1/KEY_B dallarini ters cevirmek,
 * `saved`i blend register'larindan once yuklemek (448 komuta dusuyor),
 * `ime`yi int yapmak, DMA3 icin yerel isaretci (501).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/ui/menu_screen.c
 */

#include "gba_io.h"
#include "ram_symbols.h"

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

/* Gorev listesi acikken menuye girilmeye calisilinca gosterilen kayitlar. */
#define MSG_MISSION_BUSY 0x209
#define MSG_NO_SAVE      0x221
#define MSG_QUIT_MISSION 0x222
#define MSG_NEED_CASH    0x223
#define QUIT_MISSION_FEE 1000

/* Oyuncu ilerleme blogu.  ROM tabani DUZ yukleyip +0x14'u ayri ofsetle
 * okuyor (`ldr r0,[pc] / ldr r1,[r0,#20]`).  Dizi indekslemesi
 * (`((int *)gRam02025810)[5]`) taban+20'yi TEK havuz sabitine katliyordu ve
 * `ldr r1,[r0,#0]` uretiyordu; yapi uyesi biciminde katlanma olmuyor. */
typedef struct Progress {
    u8  pad00[0x14];
    s32 cash;                   /* 0x14 */
} Progress;

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

/* Ekran menuleri 664 baytlik bir dizide duruyor: 24 baytlik baslik +
 * 20 girdi * 32 bayt. RunMenuScreen tabloya mode ile indeksliyor. */
struct Menu {
    u32   centered;         /* +0 */
    Menu *parent;           /* +4 */
    u32   unk8;             /* +8 */
    u32   unk12;            /* +12 */
    int   kind;             /* +16 */
    int   itemCount;        /* +20 */
    MenuItem items[20];     /* +24 */
};

#define SCREEN_MENUS ((Menu *)0x0832C4E8)

/* Etkin gorev girdisi: +8 tur, +9 alt tur. */
typedef struct MissionEntry {
    u8 pad0[8];
    u8 kind;                /* +8 */
    u8 subtype;             /* +9 */
} MissionEntry;

#define MISSION_ACTIVE   4

extern u8   gActiveMenuItemCount;
extern MenuItem *gActiveMenuItems[20];
extern u8   gMenuPaletteSource[];
extern u8   gUnk02010C60[];
extern u8   gVBlankState;
extern u8   gLoopState;
extern u8   gGameState[];
extern u32  gRam03000098;
extern u32  gRam0300009C;
extern u16  gRam02000498;
extern u32  gRam030000A0;

extern Menu *BuildActiveMenuItems(Menu *menu);
extern void  DrawMenuItems(Menu *menu, int selected, int firstItem);
extern void  InitMenuScreen(void);
extern u32   GetTextString(int id);

extern void  FUN_080082c4(void);
extern void  FUN_08012690(void);
extern void  FUN_0803378c(int a, int b);
extern void  FUN_08004280(u32 a, u32 b);
extern void  SetTextContext(u8 *vram, u32 stride, u8 *tiles, u8 *widths,
                            u8 fontIndex, u32 halfSpacing);
extern void  FUN_08063ca4(void);
extern void  FUN_0800ab60(void);
extern void  FUN_08005fc4(int id);
extern u32   GetRecordIndex(void);
extern void  VBlankIntrWait(void);
extern void  FUN_08007fdc(void);
extern void  FUN_08063d3c(void);
extern void  FUN_080353bc(void);
extern void  FUN_08031e44(void);
extern void  FUN_080081d4(int mode);
extern void  FUN_08030b34(void);
extern void  FUN_08030b1c(int amount);
extern u32   IsSessionActive(void);
extern void  FUN_08004f74(int which);

/* 0x08001458 */
void RunMenuScreen(int mode)
{
    volatile u16 fill;
    u16 ime;
    u8 *saved;
    MissionEntry *mission;
    Menu *menu;
    MenuItem *item;
    int sel;
    int scrollTop;
    int savedSel;
    int redraw;
    int running;
    int active;

    if (gLoopState != 0)
        return;

    if (mode == 3 || mode == 13 || mode == 14 || mode == 15) {
        mission = *(MissionEntry **)gRam02000F10;
        if (gGameState[12] != 0)
            return;
        if (mission == 0)
            return;

        active = 0;
        if (mission->kind == MISSION_ACTIVE)
            active = 1;
        if (!active)
            return;

        switch (mission->subtype) {
        case 17: case 21: case 25: case 35: case 47: case 56: case 57:
            GetTextString(MSG_MISSION_BUSY);
            FUN_08030b34();
            return;
        default:
            if (mode == 13 || mode == 15) {
                GetTextString(MSG_QUIT_MISSION);
                FUN_08030b34();
            } else {
                if (((Progress *)gRam02025810)->cash <= QUIT_MISSION_FEE - 1) {
                    GetTextString(MSG_NEED_CASH);
                    FUN_08030b34();
                    return;
                }
                FUN_08030b1c(-QUIT_MISSION_FEE);
                GetTextString(MSG_NO_SAVE);
                FUN_08030b34();
            }
            FUN_080081d4(mode);
            return;
        }
    }

    FUN_08007fdc();
    if (mode == 0 && gGameState[12] != 0)
        mode = 1;

    gVBlankState = 1;
    FUN_08005fc4(SOUND_MENU_MOVE);
    FUN_08063d3c();
    FUN_080353bc();
    FUN_08031e44();
    FUN_080082c4();

    REG_BLDCNT = BLEND_ALL;
    REG_BLDALPHA = 0;
    BLEND_Y = BLEND_Y_MAX;

    saved = gMenuPaletteSource;
    {
        u16 savedIme;

        savedIme = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = PALETTE_RAM;
    REG_DMA3.dst = saved;
    REG_DMA3.control = DMA_SAVE_PALETTE;
    REG_DMA3.control;
        REG_IME = savedIme;
    }

    {
        u16 savedIme;

        savedIme = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = MENU_PALETTE_1;
    REG_DMA3.dst = PALETTE_DEST_0;
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
        REG_IME = savedIme;
    }

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = MENU_PALETTE_0;
    REG_DMA3.dst = PALETTE_DEST_1;
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
    REG_IME = ime;

    {
        u16 savedIme;

        savedIme = REG_IME;
    REG_IME = 0;
    fill = VRAM_FILL_VALUE;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = VRAM;
    REG_DMA3.control = DMA_CLEAR_VRAM;
    REG_DMA3.control;
        REG_IME = savedIme;
    }

    FUN_08012690();
    FUN_0803378c(1, 1);

    if (mode == 4) {
        FUN_08004f74(1);
        InitMenuScreen();
        return;
    }
    if (mode == 5) {
        FUN_08004f74(2);
        InitMenuScreen();
        return;
    }
    if (mode == 6) {
        FUN_08004f74(4);
        InitMenuScreen();
        return;
    }

    menu = BuildActiveMenuItems(SCREEN_MENUS + mode);
    FUN_08004280(GetRecordIndex() + 108, 0);

    SetTextContext(VRAM, MENU_TILE_COUNT, MENU_TILES_A, MENU_TILES_B,
                 MENU_TILE_WIDTH, 0);

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = MENU_PALETTE_0;
    REG_DMA3.dst = PALETTE_DEST_0;
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
    REG_IME = ime;

    {
        u16 savedIme;

        savedIme = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = MENU_PALETTE_1;
    REG_DMA3.dst = PALETTE_DEST_1;
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
        REG_IME = savedIme;
    }

    DrawMenuItems(menu, 0, 0);
    FUN_08063ca4();

    running = 1;
    sel = 0;
    scrollTop = 0;
    redraw = 0;
    savedSel = 0;

    while (running != 0 && IsSessionActive() == 0) {
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
                    menu = BuildActiveMenuItems(SCREEN_MENUS + mode);
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
                if (sel < scrollTop) {
                    scrollTop--;
                    FUN_08004280(GetRecordIndex() + 108, 0);
                }
                redraw = 1;
            }
        } else if ((gKeys & KEY_DOWN) != 0) {
            FUN_08005fc4(SOUND_MENU_MOVE);
            if (sel < gActiveMenuItemCount - 1) {
                sel++;
                if (sel > scrollTop + (MENU_VISIBLE_MAX - 1)) {
                    scrollTop++;
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
    }

    REG_DISPCNT = MENU_DISPCNT;
    InitMenuScreen();
    gRam030000A0 = gRam02000498 = gKeys = gRam0300009C = gRam03000098 = 0;
}
