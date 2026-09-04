/* Kaynak secip yarim soz okuma — 0x0803C45C-0x0803C47B
 *
 * Kip 2 ise EWRAM'daki, kip 1 ise IWRAM'daki yarim sozu donduruyor;
 * baska kipte 0. ROM iki dalda da AYNI `ldrh`'a birlesiyor, bu yuzden
 * kaynakta ortak bir isaretci yereli uzerinden okunuyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/select_word_source.c
 */

#include "gba_types.h"

#define SRC_EWRAM ((u16 *)0x020003EC)
#define SRC_IWRAM ((u16 *)0x0300009C)

/* 0x0803C45C */
u32 SelectWordSource(u32 mode)
{
    u16 *src;

    if (mode == 2) {
        src = SRC_EWRAM;
    } else {
        if (mode != 1)
            return 0;
        src = SRC_IWRAM;
    }
    return *src;
}
