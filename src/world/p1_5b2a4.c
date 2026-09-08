/* Faz 1 ikiz: 0x0805B2A4 ve 0x0805B854.
 * ROM govdeleri karsilastirildi; deneme kaniti data/phase1_evidence/. */
#include "gba_types.h"

extern void FUN_08041ef8(u16 a, s16 b);
u32 FUN_0805b2a4(void *unused, u32 a, u32 b)
{
    FUN_08041ef8((u16)a, (s16)b);
    return 1;
}
