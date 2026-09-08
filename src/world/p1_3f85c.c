/* Eylemin hedefini ve adim isaretcisini kur — 0x0803F85C.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x0803F85C.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern u32 FUN_0803f8dc__thumb(Phase1Action *);
void FUN_0803f85c(Phase1Action *action, void *target)
{
    if (action) {
        action->step = FUN_0803f8dc__thumb;
        action->done = 0;
        action->state = 0;
        action->target = target;
        action->result = 1;
    }
}
