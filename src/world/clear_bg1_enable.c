/* Clear DISPCNT bit 9 — 0x08030860-0x08030873
 *
 * Read REG_DISPCNT and mask with 0xFDFF to clear bit 9. The ROM constructs
 * the address with `0x80 << 19` (movs #128 + lsls #19), so the source must
 * use the shifted form too.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/clear_bg1_enable.c
 */

#include "gba_types.h"

#define DISPCNT   (*(vu16 *)(0x80 << 19))
#define BG1_MASK  0xFDFF

/* 0x08030860 */
void ClearBg1Enable(void)
{
    DISPCNT = DISPCNT & BG1_MASK;
}
