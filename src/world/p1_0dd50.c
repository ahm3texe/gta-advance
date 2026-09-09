/* DMA32 zero fill with interrupts preserved — 0x0800DD50.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x0800DD50.json. */
#include "gba_types.h"
#include "gba_io.h"

extern u32 gRam0201AA80[];
void FUN_0800dd50(void)
{
    volatile u32 zero;
    u16 ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = gRam0201AA80;
    REG_DMA3.control = 0x85000000 | 3;
    REG_DMA3.control;
    REG_IME = ime;
}
