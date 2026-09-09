/* BG0/BG1 setup for the menu — 0x08012750-0x0801279F
 *
 * The same volatile macro arrangement as SetupBg0Bg1
 * (src/video/setup_bg0_bg1.c); the values differ: DISPCNT 0x1740, BG0CNT
 * 0x82|0x1500, BG1CNT 0|8|0x1300. Rules 65-66 are decisive here as well.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/video/setup_bg0_bg1_menu.c
 */

#include "gba_types.h"
#include "gba_io.h"
#define DISPCNT_VALUE   0x1740
#define BG0CNT_BASE     0x82
#define BG0CNT_SCREEN   0x1500
#define BG1CNT_CHAR     8
#define BG1CNT_SCREEN   0x1300
#define BG0CNT_V        (*(vu16 *)0x04000008)
#define BG1CNT_V        (*(vu16 *)0x0400000A)
extern u16 gRam0201AECC;
void SetupBg0Bg1Menu(void)
{
    gRam0201AECC = 0;
    REG_DISPCNT = DISPCNT_VALUE;
    BG0CNT_V = BG0CNT_BASE;
    BG0CNT_V = BG0CNT_V;
    BG0CNT_V = BG0CNT_V | BG0CNT_SCREEN;
    BG1CNT_V = 0;
    BG1CNT_V = BG1CNT_V | BG1CNT_CHAR;
    BG1CNT_V = BG1CNT_V | BG1CNT_SCREEN;
}
