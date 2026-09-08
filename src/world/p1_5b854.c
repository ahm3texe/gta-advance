/* Faz 1 ikiz: 0x0805B854 ve 0x0805B2A4.
 * ROM govdeleri karsilastirildi; deneme kaniti data/phase1_evidence/. */
#include "gba_types.h"

extern void FUN_0803378c(u16 a, u16 b);
u32 FUN_0805b854(void *unused, u32 a, u32 b)
{
    FUN_0803378c((u16)a, (u16)b);
    return 1;
}
