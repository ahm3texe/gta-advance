/* Bos dongu gecikmesi — 0x08030EA0-0x08030EA9
 *
 * Altidan geriye sayan, govdesi bos bir gecikme dongusu. Sayac SIGNED
 * olmali: ROM `bge` kullaniyor, unsigned sayacta `bhi`/`bcs` cikardi
 * (docs/COMPILER.md kural 31).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/spin_delay.c
 */

#include "gba_types.h"

#define SPIN_COUNT 6

/* 0x08030EA0 */
void SpinDelay(void)
{
    s32 i;

    i = SPIN_COUNT;
    do {
        i--;
    } while (i >= 0);
}
