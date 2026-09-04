/* Iletme sarmalayicisi — 0x0803378C-0x08033797
 *
 * Govdesi yalnizca FUN_08033f18 cagrisi. Ne yaptigi BILINMIYOR, bu yuzden ad
 * degistirilmedi: sahte semantik uydurmaktansa FUN_ adi korunuyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * Argumanlar r0-r3'te zaten hazir oldugu icin sarmalayici onlara dokunmaz;
 * imza arguman almadan yazilsa da ayni baytlar cikar.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_0803378c.c
 */

#include "gba_types.h"

extern void FUN_08033f18(void);

/* 0x0803378C */
void FUN_0803378c(void)
{
    FUN_08033f18();
}
