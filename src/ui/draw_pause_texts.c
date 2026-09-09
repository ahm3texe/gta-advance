/* Pause text — 0x080023F0-0x080024B3 (196 bytes)
 *
 * STATUS: 46/85 instructions, NEAR MISS. Size matches.
 * ClearTextArea(0,64,10), then selection-dependent SetTextContext
 * (0x06008240,30,tiles A/B,widths B,128,flag) and
 * DrawText(GetTextString(148),0,64). Repeat with flag sel==0 and text 149
 * at (40,64).
 *
 * REMAINING DIFFERENCE: ROM loads SetTextContext arguments in order r2,r3,
 * stack, then r0 (VRAM pool constant) LAST. agbcc loads VRAM FIRST and uses
 * two callee-saved registers for ok/sel, versus ROM sel=r4/ok=r1. Tried
 * VRAM as void* / u8* / integer macros, cast table macros vs extern arrays,
 * a local ok vs inline sel==0 vs two macro expansions (42-46). The matching
 * call in menu_loop.c uses 0x06000000, built with movs/lsls without a pool.
 * Here the unresolved issue is placement of the pool constant among arguments.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/ui/draw_pause_texts.c
 */

#include "gba_types.h"
#define VRAM            ((void *)0x06008240)
#define MENU_TILE_COUNT 30
#define MENU_TILE_WIDTH 128
#define TILES_A         ((const void *)0x08831880)
#define WIDTHS_A        ((const void *)0x0883F880)
#define TILES_B         ((const void *)0x0883FB60)
#define WIDTHS_B        ((const void *)0x0884DB60)
#define TEXT_RESUME     148
#define TEXT_QUIT       149
extern void ClearTextArea(u32 x, u32 y, u32 rows);
extern void SetTextContext(u8 *vram, u32 stride, u8 *tiles, u8 *widths, u32 width, u32 sel);
extern u32  GetTextString(u32 index);
extern void DrawText(u32 text, int x, int y);
void DrawPauseTexts(u32 sel)
{
    ClearTextArea(0, 64, 10);
    if (sel != 0)
        SetTextContext(VRAM, MENU_TILE_COUNT, TILES_A, WIDTHS_A, MENU_TILE_WIDTH, 0);
    else
        SetTextContext(VRAM, MENU_TILE_COUNT, TILES_B, WIDTHS_B, MENU_TILE_WIDTH, sel);
    DrawText(GetTextString(TEXT_RESUME), 0, 64);
    if (sel == 0)
        SetTextContext(VRAM, MENU_TILE_COUNT, TILES_A, WIDTHS_A, MENU_TILE_WIDTH, 0);
    else
        SetTextContext(VRAM, MENU_TILE_COUNT, TILES_B, WIDTHS_B, MENU_TILE_WIDTH, sel == 0);
    DrawText(GetTextString(TEXT_QUIT), 40, 64);
}
