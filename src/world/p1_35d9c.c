/* Release the two active objects and clear the pair record — 0x08035D9C.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x08035D9C.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern void ReleaseObject(void *object);
extern void FUN_08035ebc(Phase1ReleasePair *pair);
void FUN_08035d9c(Phase1ReleasePair *pair)
{
    if (pair->active & 1) ReleaseObject(pair);
    if (pair->active & 2) ReleaseObject(pair->objects + 72);
    pair->active = 0;
    FUN_08035ebc(pair);
}
