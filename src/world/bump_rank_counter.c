/* Bumping the rank counter — 0x08066C94-0x08066D53
 *
 * Three 5-bit counters live inside the single u16 at +0x7E of the record
 * buffer.  GetRecordIndex says which record is active, and that record's
 * counter is incremented by one and saturated at 20.
 *
 * All three come out of the SAME source pattern; agbcc emits different
 * instructions depending on whether the field crosses the byte boundary:
 *   bits 1-5   -> a single byte  (ldrb/strb +0x7E)
 *   bits 6-10  -> a half word    (ldrh/strh +0x7E, it crosses the boundary)
 *   bits 11-15 -> a single byte  (ldrb/strb +0x7F)
 * So there is no need to write the three separately; the bitfield declaration
 * is enough.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/bump_rank_counter.c
 */

#include "gba_types.h"

#define COUNTER_CAP 20

typedef struct RankCounters {
    u8  pad00[0x7E];
    u16 spare  : 1;             /* bit 0 */
    u16 countA : 5;             /* bit 1-5 */
    u16 countB : 5;             /* bit 6-10 */
    u16 countC : 5;             /* bit 11-15 */
} RankCounters;

extern RankCounters gSaveBuffer;
extern s32 GetRecordIndex(void);

/* 0x08066C94 */
void BumpRankCounter(void)
{
    switch (GetRecordIndex()) {
    case 0:
        gSaveBuffer.countA++;
        if (gSaveBuffer.countA > COUNTER_CAP)
            gSaveBuffer.countA = COUNTER_CAP;
        break;
    case 1:
        gSaveBuffer.countB++;
        if (gSaveBuffer.countB > COUNTER_CAP)
            gSaveBuffer.countB = COUNTER_CAP;
        break;
    case 2:
        gSaveBuffer.countC++;
        if (gSaveBuffer.countC > COUNTER_CAP)
            gSaveBuffer.countC = COUNTER_CAP;
        break;
    }
}
