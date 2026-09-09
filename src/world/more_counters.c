/* Additional statistics counters — 0x080671B8-0x08067205
 *
 * Overflow-protected increments of u16 counters at gSaveBuffer +0x64/+0x68/
 * +0x6A. Same pattern as stat_counters.c: the intermediate must be int; u16
 * makes agbcc add an unnecessary lsls/lsrs pair.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/more_counters.c
 */

#include "gba_types.h"

typedef struct SaveCounters {
    u8  pad00[0x64];
    u16 count64;                /* +0x64 */
    u16 pad66;
    u16 count68;                /* +0x68 */
    u16 count6A;                /* +0x6A */
} SaveCounters;

extern SaveCounters gSaveBuffer;

/* 0x080671B8 */
void BumpCount6A(void)
{
    u16 old;
    int next;

    old = gSaveBuffer.count6A;
    next = old + 1;
    gSaveBuffer.count6A = next;
    if ((u16)next == 0)
        gSaveBuffer.count6A = old;
}

/* 0x080671D4 */
void BumpCount68(void)
{
    u16 old;
    int next;

    old = gSaveBuffer.count68;
    next = old + 1;
    gSaveBuffer.count68 = next;
    if ((u16)next == 0)
        gSaveBuffer.count68 = old;
}

/* 0x080671F0 */
void BumpCount64(void)
{
    u16 old;
    int next;

    old = gSaveBuffer.count64;
    next = old + 1;
    gSaveBuffer.count64 = next;
    if ((u16)next == 0)
        gSaveBuffer.count64 = old;
}
