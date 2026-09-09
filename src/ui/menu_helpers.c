/* Menu state helpers — 0x08001DC0-0x08001E2F
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/ui/menu_helpers.c
 */

#include "gba_types.h"

#define ACTIVE_MENU_ITEM_MAX 20

extern u8  gMenuPositionX;
extern u8  gActiveMenuItemCount;
extern u32 gMenuFlags;
extern u32 gMenuRuntimeState;
extern u32 gActiveMenuItems[ACTIVE_MENU_ITEM_MAX];

/* Originally unnamed: 0x080512B0 is a six-byte accessor; 0x08004280
 * is a 624-byte operation.
 */
extern u32 GetRecordIndex(void);
extern void FUN_08004280(u32 target, u32 argument);

/* 0x08001DC0 */
void ResetMenuState(void)
{
    int i;

    gMenuPositionX = 0;
    gActiveMenuItemCount = 0;

    /* agbcc converts this forward loop to a backward pointer walk,
 * matching the ROM.
 */
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
    FUN_08004280(GetRecordIndex() + 108, 0);
}
