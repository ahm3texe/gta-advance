/* Iki slot seviyesini yuzde kadar ayarla — 0x0805A070.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x0805A070.json. */
#include "gba_types.h"

extern s32 GetSlotField(u32 slot);
extern void ApplyTwoLevels(s32 value,u32 slot);
u32 FUN_0805a070(void *unused,u16 percent)
{
    s32 value = GetSlotField(1);
    s32 delta = ((s32)(percent << 16)) / 100;
    value = value - delta;
    ApplyTwoLevels(value,1);
    ApplyTwoLevels(GetSlotField(2) - delta,2);
    return 1;
}
