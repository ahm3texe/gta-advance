/* Phase 1 twin: 0x080500DC and 0x0804FEE4.
 * Compared against the ROM bodies; attempt evidence in data/phase1_evidence/. */
#include "gba_types.h"

extern u32 GetBaseAlt(void);
extern u32 FUN_0804bfbc(void *actor);
u32 FUN_080500dc(void *actor)
{
    u32 result;
    if (GetBaseAlt()) result = FUN_0804bfbc(actor);
    else result = 1;
    return result;
}
