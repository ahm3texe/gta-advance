/* Iletme sarmalayicisi — 0x080101D8-0x080101E3
 *
 * Govdesi yalnizca FUN_0800eae0 cagrisi. Ne yaptigi BILINMIYOR, bu yuzden ad
 * degistirilmedi: sahte semantik uydurmaktansa FUN_ adi korunuyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * Argumanlar r0-r3'te zaten hazir oldugu icin sarmalayici onlara dokunmaz;
 * imza arguman almadan yazilsa da ayni baytlar cikar.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_080101d8.c
 */

#include "gba_types.h"

extern void FUN_0800eae0(void);

/* 0x080101D8 */
void FUN_080101d8(void)
{
    FUN_0800eae0();
}
