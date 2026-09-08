/* Soft-float karsilastirma; NaN durumunu ayri isle — 0x0806D900.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x0806D900.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern void FUN_0806cee8(double *,Phase1DoubleParts *);
extern s32 FUN_0806d6dc(Phase1DoubleParts *,Phase1DoubleParts *);
static inline u32 IsUnordered(Phase1DoubleParts *parts) { return parts->kind <= 1; }
s32 FUN_0806d900(double a,double b)
{
    Phase1DoubleParts left,right;
    FUN_0806cee8(&a,&left);
    FUN_0806cee8(&b,&right);
    if (IsUnordered(&left) || IsUnordered(&right)) return -1;
    return FUN_0806d6dc(&left,&right);
}
