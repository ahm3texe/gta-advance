/* Release an entry that is invalid or whose key has changed — 0x08025FC0.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x08025FC0.json. */
#include "gba_types.h"
#include "phase1_types.h"

/* table_entries.c / step_entry_timer.c ile BIREBIR AYNI govde (tutarlilik
 * kapisi ayni sembol icin ayni struct govdesini sart kosuyor). Phase1Entry
 * ile ayni 148 baytlik yerlesim: timer=unk02, object[136]=+0x04..+0x8B,
 * state=unk8C, key=phase. */
typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01;
    u16 unk02;                  /* +0x02 */
    u32 unk04;                  /* +0x04 (serbest birakilacak blok) */
    u8  pad08[0x22];
    u8  mark;                   /* +0x2A */
    u8  state;                  /* +0x2B */
    u8  pad2C[0x38];
    u8  tableIndex;             /* +0x64 */
    u8  pad65[0x27];
    u32 unk8C;                  /* +0x8C */
    u32 phase;                  /* +0x90 */
} Entry;
extern Entry gRam020246F0[20];
extern void ReleaseObject(void *);
static inline void ReleaseEntry(s16 index)
{
    Entry *entry;
    if (index == 255) return;
    entry = &gRam020246F0[index];
    if ((u16)index < 20 && entry->active) {
        ReleaseObject((u8 *)&entry->unk04);
        entry->active = 0;
        entry->unk8C = 0;
        entry->unk02 = 0;
    }
}
u32 FUN_08025fc0(Phase1EntryOwner *owner,u32 key)
{
    u8 *data = owner->data;
    Entry *entry;
    if (data[167] != 255) {
        entry = &gRam020246F0[data[167]];
        if (entry->active && entry->phase == key) return 1;
        ReleaseEntry(data[167]);
        data[167] = 255;
    }
    return 0;
}
