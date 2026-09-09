/* Collect visible menu items and compute horizontal layout — 0x0800114C-0x080011EC
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/ui/build_active_menu_items.c
 */

#include "gba_types.h"

#define ACTIVE_MENU_ITEM_MAX 20

/* Layout constants: items are 16 pixels wide. Centered menus align to 96
 * (the original note recorded 240/2 + 24), left-aligned menus to 80.
 */
#define MENU_ITEM_WIDTH      16
#define MENU_VISIBLE_MAX     8
#define MENU_CENTER_X        96
#define MENU_LEFT_X          80

typedef struct MenuItem {
    u8   unk0[12];          /* +0 text/icon fields */
    u32 *conditionFlags;    /* +12 non-null pointer controls visibility */
    u32  requiredBits;      /* +16 bits tested in conditionFlags */
    u8   unk20[12];         /* +20 */
} MenuItem;                 /* 32 bytes */

typedef struct Menu {
    u32 centered;           /* +0 nonzero selects centering */
    u8  unk4[16];           /* +4 */
    int itemCount;          /* +20 */
    MenuItem items[1];      /* +24 itemCount entries, 32 bytes each */
} Menu;

extern u8       gActiveMenuItemCount;
extern u8       gMenuPositionX;
extern MenuItem *gActiveMenuItems[ACTIVE_MENU_ITEM_MAX];

/* menu_helpers.c defines void FinalizeMenuLayout(void) and does not use r0.
 * However, this ROM call site has adds r0,r4,#0, passing the menu pointer.
 * An argument-free declaration omits that instruction, so this site uses
 * a one-parameter declaration.
 */
extern void FinalizeMenuLayout(Menu *menu);

/* 0x0800114C */
Menu *BuildActiveMenuItems(Menu *menu)
{
    int i;
    int width;

    gActiveMenuItemCount = 0;

    /* Unconditional items are always visible. Conditional items are included
 * when at least one requested flag bit is set.
 */
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
