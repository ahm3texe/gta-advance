/* Read the alternative model's value with a one-in-four chance — 0x08038644.
 *
 * BYTE-MATCHING (60/60).
 *
 * The twin of src/world/p1_38608.c and it matches for the same reason: the ROM
 * loads the table base before the index scaling in the first lookup and after
 * it in the tail, so the tail must not route the base through a local. See that
 * file for the measurement.
 *
 * One extra trap here, where the tail also reads a field. These two are the
 * same C expression by the standard and produce DIFFERENT code:
 *     gRom08CA7E64[index].value      6 instructions off
 *     (gRom08CA7E64 + index)->value  byte-matching
 * The subscript form reintroduces the early pool load; the pointer form does
 * not. Taking the element into a local first and then reading the field also
 * matches.
 *
 * Phase 1; compared against the ROM twins. Twin: src/world/p1_38608.c.
 * Measurement: data/phase1_evidence/0x08038644.json.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/p1_38644.c
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

u16 FUN_08038644(u32 index)
{
    const Phase1Model *model = LookupModel(index);
    if (model->alternate != 0 && (FUN_08032548() & 3) == 0)
        index = model->alternate;
    return (gRom08CA7E64 + index)->value;
}
