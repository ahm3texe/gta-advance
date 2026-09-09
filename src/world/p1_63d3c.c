/* Phase 1 twin: 0x08063D3C and 0x08063C88.
 * Compared against the ROM bodies; attempt evidence in data/phase1_evidence/. */
#include "gba_types.h"

extern u32 gRam02036110;
extern void FUN_08062a0c(void);
void FUN_08063d3c(void)
{
    if (gRam02036110 != 0) {
        FUN_08062a0c();
        gRam02036110 = 0;
    }
}
