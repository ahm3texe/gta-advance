/* Oyuncu girdisi veya bagli nesne bayragini denetle — 0x0803C6B8.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x0803C6B8.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern u32 GetOwnerSlot(void *);
extern u32 gRam0300009C;
extern u8 gUnk02010C60[];
u32 FUN_0803c6b8(Phase1LinkedActor *actor)
{
    Phase1GroupedActor *linked = actor->linked;
    if (GetOwnerSlot(actor)) {
        if (gRam0300009C & (gUnk02010C60[26] ? 4 : 3)) return 1;
        if (gRam0300009C & 240) return 1;
    } else if (linked->flags & 0x01000000) return 1;
    return 0;
}
