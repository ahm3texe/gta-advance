/* Communication flag — 0x08066A40-0x08066A53
 *
 * In the structure pointed to by gRam02036328 (see the ROM at 0x08066A40),
 * set byte6 to 1 when byte0 != 0.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/comm_flag.c
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
