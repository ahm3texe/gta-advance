/* Set up the two objects from their ROM definitions — 0x08035DE4.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x08035DE4.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern Phase1RomNode gRom08BD3448;
extern void FUN_08013cfc(void *,Phase1RomNode *,u32);
extern void FUN_08014ee4(void *,u32);
void FUN_08035de4(u8 *pair)
{
    Phase1RomNode *group = gRom08BD3448.slots[64];
    Phase1RomNode *desc = group->slots[4]->slots[0];
    FUN_08013cfc(pair,desc,0);
    FUN_08014ee4(pair,desc->value);
    desc = group->slots[5]->slots[0];
    pair += 72;
    FUN_08013cfc(pair,desc,0);
    FUN_08014ee4(pair,desc->value);
}
