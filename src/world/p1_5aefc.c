/* Sahibe gore nesne turu eslesmesini denetle — 0x0805AEFC.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x0805AEFC.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern u32 GetOwnerSlot(void *);
u32 FUN_0805aefc(Phase1Actor *actor)
{
    if (actor && actor->desc && (actor->kind == 8 || actor->kind == 2)) {
        if (actor->desc->id == 0x400d) return 1;
        if (GetOwnerSlot(actor)) {
            if (actor->desc->kind == 16) return 1;
        } else if (actor->desc->kind == 0x400d) return 1;
    }
    return 0;
}
