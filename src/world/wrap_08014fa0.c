/* Iletme sarmalayicisi — 0x08014FA0-0x08014FAB
 *
 * Govdesi yalnizca FUN_08014fac cagrisi. Ne yaptigi BILINMIYOR, bu yuzden ad
 * degistirilmedi: sahte semantik uydurmaktansa FUN_ adi korunuyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * Argumanlar r0-r3'te zaten hazir oldugu icin sarmalayici onlara dokunmaz;
 * imza arguman almadan yazilsa da ayni baytlar cikar.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_08014fa0.c
 */

#include "gba_types.h"

extern void FUN_08014fac(void);

/* 0x08014FA0 */
void FUN_08014fa0(void)
{
    FUN_08014fac();
}
