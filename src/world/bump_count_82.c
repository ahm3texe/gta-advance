/* Save counter bracketed by delays — 0x08067184-0x080671B7
 *
 * The same overflow-protected increment as stat_counters.c, applied to +0x82
 * in the save buffer between two empty-loop delays. SpinDelay does NOT use
 * its argument (0x08030EA0, byte-matching, src/world/spin_delay.c), but the
 * call site loads a constant into r0, so its declaration here needs an argument.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/bump_count_82.c
 */

#include "gba_types.h"

typedef struct SaveCounters82 {
    u8  pad00[0x82];
    u16 count82;                /* +0x82 */
} SaveCounters82;

extern SaveCounters82 gSaveBuffer;
extern void SpinDelay(s32 tag);

/* 0x08067184 */
void BumpCount82(void)
{
    u16 old;
    int next;

    SpinDelay(401);

    old = gSaveBuffer.count82;
    next = old + 1;
    gSaveBuffer.count82 = next;
    if ((u16)next == 0)
        gSaveBuffer.count82 = old;

    SpinDelay(403);
}
