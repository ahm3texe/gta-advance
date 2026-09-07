/* Havuz girisini birakma (E havuzu) — 0x080294B4-0x080294E3
 *
 * ReleaseEntryD (src/world/release_entry_d.c) ile KOMUT KOMUT ayni;
 * tek fark taban tablo (gRam020254D0). tools/find_twins.py bunu
 * %95.8 benzerlikle isaret etti, kardesin kaynagi kopyalanip taban
 * degistirildi (docs/WORKFLOW.md §10).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/release_entry_e.c
 */

#include "gba_types.h"

typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01;
    u16 timer;                  /* +0x02 */
    u8  sub[38];                /* +0x04, FUN_08013abc'ye verilir */
    u8  pad2a[106];             /* toplam 148 = 0x94 */
} Entry;

extern Entry gRam020254D0[];    /* 5 giris */

extern void FUN_08013abc(u8 *sub);

/* 0x080294B4 */
u32 ReleaseEntryE(u8 idx)
{
    Entry *e;

    e = &gRam020254D0[idx];
    if (e->active == 0)
        return 0;

    FUN_08013abc(e->sub);
    e->active = 0;
    e->timer = 0;
    return 1;
}
