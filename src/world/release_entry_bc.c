/* Havuz girisini birakma (B ve C havuzlari) — 0x08028B2C-0x08028B9B
 *
 * Iki fonksiyon KOMUT KOMUT ayni; yalniz taban tablo farkli. Giris etkin
 * degilse 0 doner; etkinse alt nesneyi ReleaseObject'ye birakip +0x00,
 * +0x8C ve +0x02'yi sifirlar ve 1 doner.
 *
 * +0x8C Thumb'in `str` anlik uzakligini (en fazla 124) astigi icin ROM
 * ayri bir taban uretiyor (adds r1,r4,#0 / adds r1,#140); bu C'de dogal
 * alan yazimindan kendiliginden cikiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/release_entry_bc.c
 */

#include "gba_types.h"

/* Kardes dosyalarla (entries_b1.c, entries_b5.c) ayni 148 = 0x94 adim;
 * bu iki fonksiyonun dokundugu alanlar acildi. */
typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01;
    u16 timer;                  /* +0x02 */
    u8  sub[38];                /* +0x04, ReleaseObject'ye verilir */
    u8  pad2a[98];
    u32 unk8c;                  /* +0x8C */
    u8  pad90[4];
} Entry;

extern Entry gRam02024350[];    /* 4 giris */
extern Entry gRam020245B0[];    /* 1 giris */

extern void ReleaseObject(u8 *sub);

/* 0x08028B2C */
u32 ReleaseEntryB(u8 idx)
{
    Entry *e;

    e = &gRam02024350[idx];
    if (e->active == 0)
        return 0;

    ReleaseObject(e->sub);
    e->active = 0;
    e->unk8c = 0;
    e->timer = 0;
    return 1;
}

/* 0x08028B64 */
u32 ReleaseEntryC(u8 idx)
{
    Entry *e;

    e = &gRam020245B0[idx];
    if (e->active == 0)
        return 0;

    ReleaseObject(e->sub);
    e->active = 0;
    e->unk8c = 0;
    e->timer = 0;
    return 1;
}
