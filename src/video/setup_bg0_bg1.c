/* BG0/BG1 setup — 0x080127A8-0x080127FB
 *
 * DISPCNT 0x0300 (mode 0, BG0+BG1 on), BG0CNT 0xC003 | 0x1A00 (priority 3,
 * size 3, screen base 13), BG1CNT 0x4002 | 0x1E00 (priority 2, size 1,
 * screen base 15).
 *
 * TWO THINGS ARE DECISIVE HERE:
 *
 * 1) The control register must be written through an ABSOLUTE MACRO, NOT
 *    through a pointer variable. With a pointer variable agbcc lifts
 *    0x04000008 into a common subexpression as 0x04000000 + 8
 *    (`adds r1,#8`) and the 0x04000008 entry disappears from the pool. With
 *    an absolute macro the address stays a MEM address, never becomes a
 *    pseudo-register, and CSE does not see it.
 *
 * 2) The registers must be `vu16`. REG_BG0CNT/REG_BG1CNT in include/gba_io.h
 *    are `u16 *` (NOT volatile); with those, the ROM's read-back/write pairs
 *    (ldrh/strh) are eliminated entirely. That is why this file defines its
 *    own volatile macros.
 *
 * `BG0CNT_V = BG0CNT_V;` really is present in the ROM (ldrh + strh). The value
 *    does not change, but because it is volatile the two bus operations
 *    remain.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/video/setup_bg0_bg1.c
 */

#include "gba_types.h"
#include "gba_io.h"

#define DISPCNT_VALUE   0x0300
#define BG0CNT_BASE     0xC003
#define BG0CNT_SCREEN   0x1A00
#define BG1CNT_BASE     0x4002
#define BG1CNT_SCREEN   0x1E00

#define BG0CNT_V        (*(vu16 *)0x04000008)
#define BG1CNT_V        (*(vu16 *)0x0400000A)

extern u16 gRam0201AECC;

/* 0x080127A8 */
void SetupBg0Bg1(void)
{
    REG_DISPCNT = DISPCNT_VALUE;

    BG0CNT_V = BG0CNT_BASE;
    BG0CNT_V = BG0CNT_V;
    BG0CNT_V = BG0CNT_V | BG0CNT_SCREEN;

    BG1CNT_V = BG1CNT_BASE;
    BG1CNT_V = BG1CNT_V;
    BG1CNT_V = BG1CNT_V | BG1CNT_SCREEN;

    gRam0201AECC = 0;
}
