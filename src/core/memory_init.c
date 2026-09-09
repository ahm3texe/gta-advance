/* Allocate two work buffers and clear the related state fields.
 *
 * Two allocations of 0x1000 and 0x2400 bytes are made from the 0x02014ED0
 * base. The second buffer is written into the 0x03000028 pointer in IWRAM.
 * Then two EWRAM states are cleared, the 0x102 words at 0x0201AAB0 are cleared
 * with DMA3, and the 0x02000D14 state is cleared.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm
 * Verification: make c-match FILE=src/core/memory_init.c
 */

#include "gba_io.h"

#define DMA_FILL_258_WORDS 0x85000102

extern void *FUN_0800c700(void *base, u32 size);
extern u8    gBufferBase02014ED0[];
extern void *gBufferA0201AAA0;
extern void *gBufferB03000028;
extern u32   gBufferStateA0201AAA8;
extern u32   gBufferStateB0201AAA4;
extern u8    gBufferClear0201AAB0[];
extern u32   gGlobalState02000D14;

/* 0x080100D0 */
void InitWorkBuffers(void)
{
    void **bufferA;
    void *base;
    u16 ime;
    volatile u32 zero;

    bufferA = &gBufferA0201AAA0;
    base = gBufferBase02014ED0;
    *bufferA = FUN_0800c700(base, 0x1000);
    gBufferB03000028 = FUN_0800c700(base, 0x2400);
    gBufferStateA0201AAA8 = 0;
    gBufferStateB0201AAA4 = 0;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = gBufferClear0201AAB0;
    REG_DMA3.control = DMA_FILL_258_WORDS;
    (void)REG_DMA3.control;
    REG_IME = ime;

    gGlobalState02000D14 = 0;
}
