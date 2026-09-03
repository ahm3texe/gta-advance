/* Menu durum yardimcilari — 0x08001DC0-0x08001E2F
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/ui/menu_helpers.c
 */

#include "gba_types.h"

#define ACTIVE_MENU_ITEM_MAX 20

extern u8  gMenuPositionX;
extern u8  gActiveMenuItemCount;
extern u32 gMenuFlags;
extern u32 gMenuRuntimeState;
extern u32 gActiveMenuItems[ACTIVE_MENU_ITEM_MAX];

/* Henuz adlandirilmadi: 0x080512B0 alti byte'lik bir erisimci,
 * 0x08004280 ise 624 byte'lik bir islem. */
extern u32 FUN_080512b0(void);
extern void FUN_08004280(u32 target, u32 argument);

/* 0x08001DC0 */
void ResetMenuState(void)
{
    int i;

    gMenuPositionX = 0;
    gActiveMenuItemCount = 0;

    /* agbcc bu ileri donguyu geriye giden bir isaretci yuruyusune cevirir;
     * ROM'daki bicim odur. */
    for (i = 0; i < ACTIVE_MENU_ITEM_MAX; i++)
        gActiveMenuItems[i] = 0;

    gMenuFlags = 0;
    gMenuRuntimeState = 0;
}

/* 0x08001DF8 */
u32 IsMenuFlagSet(u32 bit)
{
    if (gMenuFlags & (1 << bit))
        return 1;

    return 0;
}

/* 0x08001E1C */
void FinalizeMenuLayout(void)
{
    FUN_08004280(FUN_080512b0() + 108, 0);
}
