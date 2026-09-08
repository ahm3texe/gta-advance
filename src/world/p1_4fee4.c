/* Faz 1 ikiz: 0x0804FEE4 ve 0x080500DC.
 * ROM govdeleri karsilastirildi; deneme kaniti data/phase1_evidence/. */
#include "gba_types.h"

extern u32 GetBaseAlt(void);
extern u32 FUN_08044804(void *actor);
u32 FUN_0804fee4(void *actor)
{
    u32 result;
    if (GetBaseAlt()) result = FUN_08044804(actor);
    else result = 1;
    return result;
}
