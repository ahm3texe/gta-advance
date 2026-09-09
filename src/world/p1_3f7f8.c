/* Set up the action's target and step pointer — 0x0803F7F8.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x0803F7F8.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern u32 FUN_0803f910__thumb(Phase1Action *);
void FUN_0803f7f8(Phase1Action *action, void *target)
{
    if (action) {
        action->step = FUN_0803f910__thumb;
        action->done = 0;
        action->state = 0;
        action->target = target;
        action->result = 1;
    }
}
