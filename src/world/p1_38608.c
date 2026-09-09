/* Pick the alternative model with a one-in-four chance — 0x08038608.
 *
 * BYTE-MATCHING (60/60).
 *
 * The table base is loaded from the literal pool twice, and the ROM places the
 * two loads DIFFERENTLY. In the first lookup it loads the base before scaling
 * the index; in the tail it scales first and loads the base last:
 *     first : ldr r0,[pc,#48] / lsls r1,r4,#2 / adds r1,r1,r4 / lsls r1,r1,#4
 *     tail  : lsls r0,r4,#2 / adds r0,r0,r4 / lsls r0,r0,#4 / ldr r1,[pc,#8]
 * So the original source cannot have written the two lookups the same way.
 *
 * WHAT DECIDES IT: whether the base passes through a local. Assigning it to one
 * (`table = gRom08CA7E64; return table + index;`) materialises the pool load at
 * the assignment, ahead of the scaling. Using the symbol directly in the
 * returned expression leaves the load at its point of use, after the scaling.
 * Hence the asymmetry here: the first lookup keeps the inline helper with its
 * local, the tail does not.
 *
 * Measured, all against 60 bytes: helper in both places 1 instruction off (the
 * tail's load early); helper dropped from both places 6 off (the FIRST load
 * moves late as well); helper first + direct tail 0. Three spellings of the
 * direct tail are equivalent and all match: `gRom08CA7E64 + index`,
 * `&gRom08CA7E64[index]`, and scaling into a separate offset variable.
 *
 * Phase 1; compared against the ROM twins. Twin: src/world/p1_38644.c.
 * Measurement: data/phase1_evidence/0x08038608.json.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/p1_38608.c
 */
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
    return gRom08CA7E64 + index;
}
