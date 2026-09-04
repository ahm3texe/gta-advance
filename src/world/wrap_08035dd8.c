/* Iletme sarmalayicisi — 0x08035DD8-0x08035DE3
 *
 * Govdesi yalnizca FUN_08035a98 cagrisi. Ne yaptigi BILINMIYOR, bu yuzden ad
 * degistirilmedi: sahte semantik uydurmaktansa FUN_ adi korunuyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * Argumanlar r0-r3'te zaten hazir oldugu icin sarmalayici onlara dokunmaz;
 * imza arguman almadan yazilsa da ayni baytlar cikar.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_08035dd8.c
 */

#include "gba_types.h"

extern void FUN_08035a98(void);

/* 0x08035DD8 */
void FUN_08035dd8(void)
{
    FUN_08035a98();
}
