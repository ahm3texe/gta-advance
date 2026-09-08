/* Grup uyelerini veya tek nesneyi sirayla ver — 0x0805AD10.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x0805AD10.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern Phase1GroupedActor *ResolveRecordNodeChain(u16 id);
u32 FUN_0805ad10(Phase1GroupedActor *actor,s32 *cursor,Phase1GroupedActor **out)
{
    u32 result;
    Phase1RecordGroup *group = actor->group;
    if (actor == 0) return 0;
    if (group->count == 0) {
        if (*cursor == 0) {
            *cursor = 1;
            *out = actor;
            return 1;
        }
        *out = 0;
        return 0;
    }
    if (*cursor >= group->count) {
        *out = 0;
        result = 0;
    } else {
        *out = ResolveRecordNodeChain(group->ids[*cursor]);
        (*cursor)++;
        result = 1;
    }
    return result;
}
