/* Soft-float comparison; handle the NaN case separately — 0x0806D810.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x0806D810.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern void FUN_0806cee8(double *,Phase1DoubleParts *);
extern s32 FUN_0806d6dc(Phase1DoubleParts *,Phase1DoubleParts *);
static inline u32 IsUnordered(Phase1DoubleParts *parts) { return parts->kind <= 1; }
s32 FUN_0806d810(double a,double b)
{
    Phase1DoubleParts left,right;
    FUN_0806cee8(&a,&left);
    FUN_0806cee8(&b,&right);
    if (IsUnordered(&left) || IsUnordered(&right)) return 1;
    return FUN_0806d6dc(&left,&right);
}
