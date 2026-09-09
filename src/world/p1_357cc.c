/* DMA32 zero fill with interrupts preserved — 0x080357CC.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x080357CC.json. */
#include "gba_types.h"
#include "gba_io.h"

extern u32 gRam02027F00[];
void FUN_080357cc(void)
{
    volatile u32 zero;
    u16 ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = gRam02027F00;
    REG_DMA3.control = 0x85000000 | 144;
    REG_DMA3.control;
    REG_IME = ime;
}
