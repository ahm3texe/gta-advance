/* Empty-loop delay — 0x08030EA0-0x08030EA9
 *
 * Count down from six with an empty body. The counter must be SIGNED: the
 * ROM uses bge; unsigned would produce bhi/bcs (COMPILER.md rule 31).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/spin_delay.c
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
