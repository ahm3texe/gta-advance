/* Statistics counters — 0x08067228-0x08067273
 *
 * Three counters reside in gSaveBuffer at +0x6C/+0x6E/+0x74 and are therefore
 * saved with the game. The first two perform overflow-protected increments:
 * if u16 wraps to zero, restore the old value.
 *
 * The fourth cluster member, AddDistance, is separate and byte-matching:
 * src/world/distance_accum.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/stat_counters.c
 */

#include "gba_types.h"

#define ACCUM_MASK   0x1FFF
#define ACCUM_SHIFT  13
#define DELTA_SHIFT  16

/* Save-buffer counter fields; other views of the complete layout are in
 * src/world/entity_flags.c and src/save/save_manager.c.
 */
typedef struct SaveCounters {
    u8  pad00[0x6C];
    u16 countA;                 /* +0x6C */
    u16 countB;                 /* +0x6E */
    u8  pad70[4];
    u16 distance;               /* +0x74 */
} SaveCounters;

extern SaveCounters gSaveBuffer;
extern u32 gDistanceAccum;

extern void Memset(void *dest, int value, u32 size);

/* 0x08067228 */
void BumpCountB(void)
{
    u16 old;
    int next;

    old = gSaveBuffer.countB;
    next = old + 1;
    gSaveBuffer.countB = next;
    if ((u16)next == 0)
        gSaveBuffer.countB = old;
}

/* 0x08067244 */
void BumpCountA(void)
{
    u16 old;
    int next;

    old = gSaveBuffer.countA;
    next = old + 1;
    gSaveBuffer.countA = next;
    if ((u16)next == 0)
        gSaveBuffer.countA = old;
}

/* 0x08067260 */
void ResetDistanceAccum(void)
{
    Memset(&gDistanceAccum, 0, 8);
}
