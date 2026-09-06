/* Iletisim bayragi — 0x08066A40-0x08066A53
 *
 * `gRam02036328`'in isaret ettigi struct (bkz. 0x08066A40 ROM) icinde
 * byte0 != 0 iken byte6'yi 1 yapiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/comm_flag.c
 */

#include "gba_types.h"
#include "comm_block.h"


/* 0x08066A40 */
void MaybeSetCommByte6(void)
{
    CommBlock *b;

    b = gRam02036338;
    if (b->byte0 != 0)
        b->ready06 = 1;
}
