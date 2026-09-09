/* Check the object kind match against the owner — 0x0805AB90.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x0805AB90.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern u32 GetOwnerSlot(void *);
u32 FUN_0805ab90(Phase1Actor *actor)
{
    if (actor && actor->desc && (actor->kind == 8 || actor->kind == 2)) {
        if (actor->desc->id == 0x4009) return 1;
        if (GetOwnerSlot(actor)) {
            if (actor->desc->kind == 14) return 1;
        } else if (actor->desc->kind == 0x4009) return 1;
    }
    return 0;
}
