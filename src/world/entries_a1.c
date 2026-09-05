/* FUN_08028df4 — 0x08028DF4-0x08028E39
 *
 * gEntriesA tablosunu (15 giris, 148 bayt stride) bastan sona tariyor.
 * Bir giris etkinse (+0x00 sifirdan farkli) uc kosuldan biri tutuyor mu
 * diye bakiyor: tur alani (+0x64) 0x33 veya 0x4C mi, ya da +0x90'daki
 * kelime 46 mi. Herhangi biri tutarsa 1, hicbiri tutmazsa 0 donuyor.
 *
 * Kardesi HasWantedEntry (0x08028E3C, src/world/kind_scan.c) ile ayni
 * kalip: taban ayri yerelde tutulup ondan uc yurutucu isaretci
 * turetiliyor (kural 37). ROM'daki kurulum sirasi kaynak sirasiyla
 * birebir ayni: +0x90, +0x64, taban, bitis.
 *
 * Bitis isaretcisi ROM'da `adds r4, r2, r0` ile TABANDAN kuruluyor
 * (havuz sabiti 0x8A8), karsilastirma ise +0x90 yurutucusuyle yapiliyor;
 * bu yuzden `end = cur + 0x8A8` yazildi.
 *
 * Prolog `push {r4, lr}` -> bes canli deger (taban, uc yurutucu, bitis),
 * docs/COMPILER.md register tablosuyla uyumlu. Donus `pop {r4}; pop {r1};
 * bx r1` ve r0 canli -> DEGER donduruyor (kural 35'in tersi, alloc_node.c
 * ile ayni).
 *
 * ESLESME: 70/70 bayt, ilk denemede. Kardes fonksiyonun kalibi (kural 37 +
 * kaynak sirasinin korunmasi, kural 19) dogrudan uydu; ayrica bir varyant
 * denemesi gerekmedi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_a1.c
 */

#include "gba_types.h"

#define ENTRY_STRIDE  148
#define END_OFFSET    0x8A8

#define KIND_A        0x33
#define KIND_B        0x4C
#define EXTRA_WANTED  46

/* src/world/kind_scan.c'deki Entry ile ayni yerlesim; +0x90 alani burada
 * ilk kez gorunduyu icin eklendi (stride yine 148). */
typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01[99];
    u8  kind;                   /* +0x64 */
    u8  pad65[43];
    u32 extra;                  /* +0x90 */
} Entry;

extern Entry gEntriesA[];

/* 0x08028DF4 */
u32 FUN_08028df4(void)
{
    u8 *base;
    u8 *extra;
    u8 *kind;
    u8 *cur;
    u8 *end;

    base  = (u8 *)gEntriesA;
    extra = base + 0x90;
    kind  = base + 0x64;
    cur   = base;
    end   = cur + END_OFFSET;

    do {
        if (*cur != 0) {
            if (*kind == KIND_A || *kind == KIND_B ||
                *(u32 *)extra == EXTRA_WANTED)
                return 1;
        }
        extra += ENTRY_STRIDE;
        kind  += ENTRY_STRIDE;
        cur   += ENTRY_STRIDE;
    } while (extra <= end);

    return 0;
}
