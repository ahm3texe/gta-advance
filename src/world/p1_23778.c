/* Phase 1 twin: 0x08023778 and 0x08023794.
 * Compared against the ROM bodies; attempt evidence in data/phase1_evidence/. */
#include "gba_types.h"

extern u32 GetOwnerSlot(void *owner);
extern u8 *SelectSlotAB(u32 slot);
void FUN_08023778(void *owner)
{
    u8 *slot = SelectSlotAB(GetOwnerSlot(owner));
    if (slot) slot[63] = 0;
}
