/* Faz 1 ikiz: 0x08063D3C ve 0x08063C88.
 * ROM govdeleri karsilastirildi; deneme kaniti data/phase1_evidence/. */
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
