/* Release both buffers into one sink — 0x08010224-0x08010253
 *
 * One handle comes from EWRAM and the other from IWRAM at 0x03000028, and both
 * are cleared afterwards from a single zero register. The sink's address stays
 * in a callee-saved register across both calls.
 *
 * The IWRAM address comes through the literal pool as a constant, so it is a
 * cast and not an extern (rule 1 applies to the mapped RAM symbols). It is
 * assigned BETWEEN the two calls, which is where the ROM loads it; taken at the
 * top all three pool loads come out together (rule 70).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/release_two_buffers.c
 */

#include "gba_types.h"

#define IWRAM_HANDLE  ((u32 *)0x03000028)

extern u8    gBufferBase02014ED0[];
extern void *gBufferA0201AAA0;

extern void FUN_0800c804(u32 *sink, u32 handle);

/* 0x08010224 */
void FUN_08010224(void)
{
    u32 *sink = (u32 *)gBufferBase02014ED0;
    void **first = &gBufferA0201AAA0;
    u32 *second;

    FUN_0800c804(sink, (u32)*first);
    second = IWRAM_HANDLE;
    FUN_0800c804(sink, *second);
    *first = 0;
    *second = 0;
}
