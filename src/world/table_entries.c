/* Tablo girdisi sorgu ve serbest birakma — 0x08028C48-0x08028CE7
 *
 * Uc kucuk fonksiyon; her biri 148 baytlik girisli bir tabloya `index * 148`
 * ile erisiyor. Ilk fonksiyon aktiflik bayragini test ediyor; digerleri
 * girisi kapatiyor (bir bloku serbest birakip alan sifirliyor).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/table_entries.c
 */

#include "gba_types.h"

#define ENTRY_STRIDE   148
#define INDEX_INVALID  0xFF
#define INDEX_MAX      19
#define OWNER_MASK     0x0F

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

typedef struct Owner {
    u8 pad00[0x8A];
    s8 flags;                   /* +0x8A */
} Owner;

extern Entry gRam020251EC[20];
extern Entry gRam02025280[20];
extern Entry gRam020246F0[20];

extern void ReleaseObject(u32 *block);

/* 0x08028C48 */
u32 IsEntryActive(u32 index)
{
    Entry *entry;

    if (index == 0)
        return 0;

    entry = &gRam020251EC[index];
    if (entry->active == 0)
        return 0;

    return 1;
}

/* 0x08028C68 */
u32 ReleaseEntry(Owner *owner, u32 index)
{
    Entry *entry;

    entry = &gRam02025280[index];
    if (entry->active == 0)
        return 0;

    ReleaseObject(&entry->unk04);
    entry->active = 0;
    entry->unk8C = 0;
    owner->flags &= ~OWNER_MASK;

    return 1;
}

/* 0x08028CA8 */
u32 ClearEntry(u32 idx)
{
    Entry *entry;
    s32 index;
    u32 raw;

    index = (s32)(idx << 16) >> 16;
    if (index == INDEX_INVALID)
        return 0;

    entry = &gRam020246F0[index];
    raw = (u16)idx;
    if (raw > INDEX_MAX)
        return 0;
    if (entry->active == 0)
        return 0;

    ReleaseObject(&entry->unk04);
    entry->active = 0;
    entry->unk8C = 0;
    entry->unk02 = 0;

    return 1;
}
