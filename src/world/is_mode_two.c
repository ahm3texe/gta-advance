/* Kip sorgulari — 0x0806233C-0x08062363
 *
 * 0x02036050'deki bayti 2 ile karsilastirip 1/0 donduruyor; yapi
 * IsStateReady ile ozdes, yalnizca alan bayt ve ofset +0.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/is_mode_two.c
 */

#include "gba_types.h"

#define MODE_READY 2
#define MODE_DONE  4

extern u8 gRam02036050;

/* 0x0806233C */
u32 IsModeReady(void)
{
    if (gRam02036050 == MODE_READY)
        return 1;
    return 0;
}

/* 0x08062350 */
u32 IsModeDone(void)
{
    if (gRam02036050 == MODE_DONE)
        return 1;
    return 0;
}
