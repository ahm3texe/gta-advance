/* DISPCNT bit 9 temizleme — 0x08030860-0x08030873
 *
 * REG_DISPCNT'i okuyup 0xFDFF ile maskeleyerek bit 9'u siliyor.
 * ROM adresi sabit yerine `0x80 << 19` ile kuruyor (movs #128 + lsls #19),
 * bu yuzden kaynakta da kaydirmali bicim yazilmali.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/clear_bg1_enable.c
 */

#include "gba_types.h"

#define DISPCNT   (*(vu16 *)(0x80 << 19))
#define BG1_MASK  0xFDFF

/* 0x08030860 */
void ClearBg1Enable(void)
{
    DISPCNT = DISPCNT & BG1_MASK;
}
