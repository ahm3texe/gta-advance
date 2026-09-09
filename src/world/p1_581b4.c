/* Update the state bit on the group objects — 0x080581B4.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x080581B4.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern Phase1GroupedActor *FindFreeNode(u16);
extern u32 FUN_08056508(Phase1GroupedActor *);
extern Phase1GroupedActor *GetOrCreateRecordNode(u16);
u32 FUN_080581b4(void *unused,u16 id)
{
    Phase1GroupedActor *member = FindFreeNode(id);
    Phase1GroupedActor *actor;
    s32 cursor;
    if (!member) return 0;
    member->state |= 32;
    if (member->flags & 256) {
        cursor = 0;
        actor = member;
        
        for (;;) {
            Phase1RecordGroup *group = actor->group;
            if (!(actor->flags & 256)) {
                if (cursor != 0) break;
                cursor = 1;
                member = actor;
            } else if (cursor < group->count) {
                u16 id = group->ids[cursor];
                Phase1GroupedActor *resolved = GetOrCreateRecordNode(id);
                cursor++;
                member = resolved;
            } else member = 0;
            if (!member) break;
            member->state |= 32;
        }
    }
    return 1;
}
