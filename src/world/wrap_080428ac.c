/* Iletme sarmalayicisi — 0x080428AC-0x080428B7
 *
 * Govdesi yalnizca FUN_08042900 cagrisi. Ne yaptigi BILINMIYOR, bu yuzden ad
 * degistirilmedi: sahte semantik uydurmaktansa FUN_ adi korunuyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * Argumanlar r0-r3'te zaten hazir oldugu icin sarmalayici onlara dokunmaz;
 * imza arguman almadan yazilsa da ayni baytlar cikar.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_080428ac.c
 */

#include "gba_types.h"

extern void FUN_08042900(void);

/* 0x080428AC */
void FUN_080428ac(void)
{
    FUN_08042900();
}
