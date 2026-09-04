/* Iletme sarmalayicisi — 0x0804FB4C-0x0804FB57
 *
 * Govdesi yalnizca FUN_080457f8 cagrisi. Ne yaptigi BILINMIYOR, bu yuzden ad
 * degistirilmedi: sahte semantik uydurmaktansa FUN_ adi korunuyor.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 DONUS DEGERI tasiyor, donus tipi u32.
 * Argumanlar r0-r3'te zaten hazir oldugu icin sarmalayici onlara dokunmaz;
 * imza arguman almadan yazilsa da ayni baytlar cikar.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_0804fb4c.c
 */

#include "gba_types.h"

extern u32 FUN_080457f8(void);

/* 0x0804FB4C */
u32 FUN_0804fb4c(void)
{
    return FUN_080457f8();
}
