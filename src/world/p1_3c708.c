/* Return the check kind according to slot readiness — 0x0803C708.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x0803C708.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern Phase1ReadinessActor **SelectSlotAB(u32);
extern u8 gUnk02010C60[];
u32 FUN_0803c708(u32 slot)
{
    Phase1ReadinessActor **selected = SelectSlotAB(slot);
    if (!selected || !(*selected)->state || (*selected)->state->value <= 0) return 0;
    return gUnk02010C60[26] ? 1 : 2;
}
