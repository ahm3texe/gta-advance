/* Find the counter by id and compare it against the threshold — 0x080624B8.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x080624B8.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern u32 gRam02035AD0[];
extern Phase1StatTable gRam02035ED0;
static inline u32 LookupStat(u16 id)
{
    s32 i;
    if (id == 114) return gRam02035AD0[0];
    if (id == 115) return gRam02035AD0[1];
    for (i = 0; i < 12; i++)
        if (gRam02035ED0.ids[i] == id) return gRam02035ED0.values[i];
    return 0;
}
u32 FUN_080624b8(void *unused,u16 id,u16 threshold)
{
    if (LookupStat(id) <= threshold) return 1;
    return 0;
}
