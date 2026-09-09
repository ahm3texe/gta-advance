/* Word comparison — 0x08050190-0x080501C7
 *
 * Compare two independent words: 0 means equal, 1 means different.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/word_compare.c
 */

#include "gba_types.h"

extern u32 gRam020302E0;
extern u32 gRam02030308;
extern u32 gRam02030304;
extern u32 gRam02030324;

/* 0x08050190 */
u32 CompareA(void)
{
    if (gRam020302E0 != gRam02030308)
        return 1;

    return 0;
}

/* 0x080501AC */
u32 CompareB(void)
{
    if (gRam02030324 != gRam02030304)
        return 1;

    return 0;
}
