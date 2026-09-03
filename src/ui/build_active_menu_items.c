/* Gorunur menu girdilerini toplar ve yatay yerlesimi hesaplar — 0x0800114C-0x080011EC
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/ui/build_active_menu_items.c
 */

typedef unsigned char u8;
typedef unsigned int  u32;

#define ACTIVE_MENU_ITEM_MAX 20

/* Yerlesim sabitleri: her girdi 16 piksel genis, menu ekranda ortalanir.
 * Ortali menuler 96'ya (240/2 + 24), sol menuler 80'e gore hizalanir. */
#define MENU_ITEM_WIDTH      16
#define MENU_VISIBLE_MAX     8
#define MENU_CENTER_X        96
#define MENU_LEFT_X          80

typedef struct MenuItem {
    u8   unk0[12];          /* +0  metin/ikon alanlari */
    u32 *conditionFlags;    /* +12 NULL degilse girdinin gorunurlugunu belirler */
    u32  requiredBits;      /* +16 conditionFlags icinde aranan bitler */
    u8   unk20[12];         /* +20 */
} MenuItem;                 /* 32 byte */

typedef struct Menu {
    u32 centered;           /* +0  sifir disi ise menu ortalanir */
    u8  unk4[16];           /* +4 */
    int itemCount;          /* +20 */
    MenuItem items[1];      /* +24 itemCount tane, 32'ser byte */
} Menu;

extern u8       gActiveMenuItemCount;
extern u8       gMenuPositionX;
extern MenuItem *gActiveMenuItems[ACTIVE_MENU_ITEM_MAX];

/* DIKKAT: src/ui/menu_helpers.c bu fonksiyonu `void FinalizeMenuLayout(void)`
 * olarak tanimliyor ve govdesi r0'i gercekten kullanmiyor. Ama ROM'daki cagri
 * yerinde `adds r0, r4, #0` var, yani cagiran taraf menu isaretcisini
 * geciriyor. Argumansiz bildirimle o komut uretilmiyor ve blok sapiyor;
 * bu yuzden burada tek parametreli bildirim kullanildi. */
extern void FinalizeMenuLayout(Menu *menu);

/* 0x0800114C */
Menu *BuildActiveMenuItems(Menu *menu)
{
    int i;
    int width;

    gActiveMenuItemCount = 0;

    /* Kosulsuz girdiler her zaman gorunur; kosullu olanlar ancak istenen
     * bitlerden en az biri set ise listeye girer. */
    for (i = 0; i < menu->itemCount; i++) {
        if (menu->items[i].conditionFlags == 0 ||
            (*menu->items[i].conditionFlags & menu->items[i].requiredBits) != 0) {
            gActiveMenuItems[gActiveMenuItemCount] = &menu->items[i];
            gActiveMenuItemCount++;
        }
    }

    width = menu->itemCount;
    if (width > MENU_VISIBLE_MAX)
        width = MENU_VISIBLE_MAX;
    width = width * MENU_ITEM_WIDTH;

    if (menu->centered != 0)
        gMenuPositionX = MENU_CENTER_X - (width >> 1);
    else
        gMenuPositionX = MENU_LEFT_X - (width >> 1);

    FinalizeMenuLayout(menu);
    return menu;
}
