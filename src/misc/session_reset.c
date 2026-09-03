/* Kucuk oturum yardimcilari — 0x0803C77C-0x0803C797
 *
 * 0x0803C790'daki fonksiyonu Ghidra'nin fonksiyon haritasi kacirmisti.
 * Yapilarin alan adlari bilinmiyor; ofsetler ROM'dan okundu.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/misc/session_reset.c
 */

#include "gba_types.h"

extern u8       gUnk02010C60[32];

/* 0x0803C77C */
u32 GetScaleFactor(void)
{
    u32 selector;
    u32 factor;

    selector = gUnk02010C60[26];
    factor = 1;
    if (selector != 0)
        factor += 255;

    return factor;
}

/* 0x0803C790 — Ghidra bu fonksiyonu kacirmisti */
u32 ReturnOne(void)
{
    return 1;
}

/* 0x0803C794 */
void NoOpSessionHook(void)
{
}

