/* Queue event 51 when a record is selected — 0x08063DCC-0x08063DE7
 *
 * gRam02025800 is the index into the 28-byte record table; zero means no
 * record, and then nothing is queued.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/boot/queue_event_51.c
 */

#include "gba_types.h"

#define EVENT_ID  51

extern u32 gRam02025800;

extern void FUN_08062dd8(u32 id, u32 a, u32 b);

/* 0x08063DCC */
void FUN_08063dcc(void)
{
    if (gRam02025800 == 0)
        return;
    FUN_08062dd8(EVENT_ID, 0, 0);
}
