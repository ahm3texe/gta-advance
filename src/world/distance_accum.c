/* Distance accumulator — 0x08067274-0x080672B7
 *
 * Add |delta| >> 16 on each call. When the accumulated value exceeds 0x1FFF,
 * transfer the overflow (>> 13) to the save counter and mask the accumulator.
 * The final comparison protects against overflow.
 *
 * BYTE-MATCHING. A separate accum pointer loads the base into r4 before sign
 * correction. A narrow volatile view used only in the overflow comparison
 * forces a memory reread of the stored distance field, as in the ROM.
 *
 * Three matching counters in the same cluster: src/world/stat_counters.c
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/distance_accum.c
 */

#include "gba_types.h"

#define ACCUM_MASK   0x1FFF
#define ACCUM_SHIFT  13
#define DELTA_SHIFT  16

typedef struct SaveCounters {
    u8  pad00[0x74];
    u16 distance;               /* +0x74 */
} SaveCounters;

extern SaveCounters gSaveBuffer;
extern u32 gDistanceAccum;

/* 0x08067274 */
void AddDistance(int delta)
{
    u32 *accum;
    u32 total;
    u16 old;

    accum = &gDistanceAccum;
    if (delta < 0)
        delta = -delta;

    total = *accum + (delta >> DELTA_SHIFT);
    *accum = total;

    if (total > ACCUM_MASK) {
        old = gSaveBuffer.distance;
        gSaveBuffer.distance = old + (total >> ACCUM_SHIFT);
        *accum = total & ACCUM_MASK;
        if (*(volatile u16 *)&gSaveBuffer.distance < old)
            gSaveBuffer.distance = old;
    }
}
