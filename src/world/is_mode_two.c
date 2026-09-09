/* Mode queries — 0x0806233C-0x08062363
 *
 * Compare the byte at 0x02036050 with 2 and return 1/0. The structure is
 * identical to IsStateReady, except that the field is a byte at offset +0.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/is_mode_two.c
 */

#include "gba_types.h"

#define MODE_READY 2
#define MODE_DONE  4

extern u8 gRam02036050[];

/* 0x0806233C */
u32 IsModeReady(void)
{
    if (gRam02036050[0] == MODE_READY)
        return 1;
    return 0;
}

/* 0x08062350 */
u32 IsModeDone(void)
{
    if (gRam02036050[0] == MODE_DONE)
        return 1;
    return 0;
}
