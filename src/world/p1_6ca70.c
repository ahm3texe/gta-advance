/* Soft-float comparison; handle the NaN case separately — 0x0806CA70.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x0806CA70.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern void FUN_0806c450(float *,Phase1FloatParts *);
extern s32 FUN_0806c910(Phase1FloatParts *,Phase1FloatParts *);
static inline u32 IsUnordered(Phase1FloatParts *parts) { return parts->kind <= 1; }
s32 FUN_0806ca70(float a,float b)
{
    Phase1FloatParts left,right;
    FUN_0806c450(&a,&left);
    FUN_0806c450(&b,&right);
    if (IsUnordered(&left) || IsUnordered(&right)) return 1;
    return FUN_0806c910(&left,&right);
}
