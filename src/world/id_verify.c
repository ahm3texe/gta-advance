/* Kimlik dogrulama ve tetikleme — 0x080336EC-0x0803376B
 *
 * `id` bir kompozit deger: yuksek bayt = tablo indeksi, dusuk 24 bit =
 * ROM offset. Tablo girdisinin +0 alani, `0x08000000 | offset` olmali.
 * Ilk fonksiyon "kimlik dogru mu" testi (bool), ikincisi eslesirse
 * FUN_08032A54 tetikliyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/id_verify.c
 */

#include "gba_types.h"

#define FLAG_READY   1
#define STRIDE       44
#define INDEX_SHIFT  24
#define OFFSET_MASK  0x00FFFFFF
#define ROM_BASE     0x08000000

typedef struct AddrEntry {
    u32 addr;                   /* +0x00 */
    u8  pad04[40];              /* stride 44 */
} AddrEntry;

extern u8         gCartFlag;
extern AddrEntry  gAddrTable[];

extern void FUN_08032a54(AddrEntry *entry);

/* 0x080336EC */
u32 VerifyId(u32 id)
{
    AddrEntry *entry;
    u32 expected;

    if (gCartFlag != FLAG_READY)
        return 0;
    if (id == 0)
        return 0;

    entry = &gAddrTable[id >> INDEX_SHIFT];
    expected = (id & OFFSET_MASK) | ROM_BASE;
    if (entry == 0)
        return 0;
    if (entry->addr != expected)
        return 0;

    return 1;
}

/* 0x0803372C */
void TriggerIfMatched(u32 id)
{
    AddrEntry *entry;
    u32 expected;

    if (gCartFlag != FLAG_READY)
        return;
    if (id == 0)
        return;

    entry = &gAddrTable[id >> INDEX_SHIFT];
    expected = (id & OFFSET_MASK) | ROM_BASE;
    if (entry->addr != expected)
        return;

    FUN_08032a54(entry);
}
