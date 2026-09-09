/* Pick the alternative model with a one-in-four chance — 0x08038608.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x08038608.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern const Phase1Model gRom08CA7E64[];
extern u32 FUN_08032548(void);
static inline const Phase1Model *LookupModel(u32 index)
{
    const Phase1Model *table = gRom08CA7E64;
    return table + index;
}

const Phase1Model * FUN_08038608(u32 index)
{
    const Phase1Model *model = LookupModel(index);
    if (model->alternate != 0 && (FUN_08032548() & 3) == 0)
        index = model->alternate;
    return LookupModel(index);
}
