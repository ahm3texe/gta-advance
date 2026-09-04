/* MaybeAdvance — 0x080664F0-0x08066519
 *
 * Yalniz gVBlankEnabled tam COUNTER_MIN (=2) ve sayac 1'den buyukse 0
 * donuyor; diger tum durumlarda 1. Ayni kumedeki uc kardesi
 * src/world/pause_helpers.c'de.
 *
 * BYTE-MATCHING. Onceki `<= 1 ise 0` yorumu ve C kosulu ROM'daki `bls`
 * dalinin hedefini ters okumustu. Kosulu ROM davranisiyla ayni kurmak
 * dogal C'den birebir dal yonunu uretiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/maybe_advance.c
 */

#include "gba_types.h"

#define COUNTER_MIN    2

extern u16  gVBlankEnabled;
extern u16  gRam0200048C;

extern void FUN_080657d8(u32 arg);

/* 0x080664F0 */
u32 MaybeAdvance(void)
{
    FUN_080657d8(0);
    if (gVBlankEnabled == COUNTER_MIN && gRam0200048C > 1)
        return 0;

    return 1;
}
