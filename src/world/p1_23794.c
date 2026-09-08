/* Faz 1 ikiz: 0x08023794 ve 0x08023778.
 * ROM govdeleri karsilastirildi; deneme kaniti data/phase1_evidence/. */
#include "gba_types.h"

extern u32 GetOwnerSlot(void *owner);
extern u8 *SelectSlotAB(u32 slot);
void FUN_08023794(void *owner)
{
    u8 *slot = SelectSlotAB(GetOwnerSlot(owner));
    if (slot) slot[63] = 0;
}
