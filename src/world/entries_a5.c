/* gEntriesA'da bos yuva bulup yeni giris kurma — 0x08028A88-0x08028B29
 *
 * gEntriesA tablosunda (15 giris, 148 bayt stride) +0x00 alani sifir olan
 * ilk bos yuvayi ariyor. Bos yuva yoksa hicbir sey yapmadan donuyor.
 * Buldugunda yuvayi dolduruyor: aktif bayragi 1, tur (+0x64) 34,
 * +0x90 = phase, +0x84 = owner, +0x4c'ye cagiranin verdigi 12 baytlik
 * uclu kopyalaniyor, +0x68 = arg1 << 16, +0x8c = 0. Ardindan
 * FUN_08014ffc(&e->sub, 3, &e->payload, &e->unk68) cagriliyor;
 * phase 51 ise +0x2a'ya 1, degilse 64 yaziliyor; son olarak
 * FUN_08015038(&e->sub) ve FUN_080289bc(e) cagriliyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * Kural 32: `ldmia r0!,{r3,r6,r7}` / `stmia r1!,{r3,r6,r7}` ucluyu
 *   struct atamasi olarak yazmayi gerektiriyor (`Triple`), `*d++ = *s++`
 *   degil.
 * Kural 9/31: `cmp r4,#14` + `bgt` isaretli dal -> sayac `s32`.
 * Kural 42: ROM dongude isaretci yurutuyor ama C'de artan indeksli
 *   `for` yazildi; agbcc guclendirmeyle isaretci yurutucusunu kendisi
 *   uretiyor, dongu sonrasi `&gEntriesA[i]` ise `muls #148` ile
 *   yeniden kuruluyor -- ROM'un tam yaptigi.
 *
 * Dongu cikis dali dogrudan epiloga gidiyor (0x8028AA8 -> 0x8028B20),
 * cunku agbcc jump-threading ile ayni kosulu (`i > 14`) tekrar test eden
 * blogu atliyor; kaynakta ayri `break` + `if (i > 14) return;` yazmak
 * bu ikili `cmp`/`bgt` desenini uretiyor.
 *
 * Taban havuzdan DUZ yukleniyor (0x02023A00) ve tum alan ofsetleri
 * `adds` ile ayri kuruluyor -> dizi aritmetigi degil, yapi uyesi erisimi;
 * bu yuzden yuva isaretcisi `Entry *e` olarak tutuldu.
 *
 * ESLESME: 162/162 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_a5.c
 */

#include "gba_types.h"

#define ENTRY_COUNT_MAX  14      /* i <= 14, yani 15 giris */
#define WANTED_KIND      34      /* +0x64 */
#define PHASE_SPECIAL    51      /* phase == 51 ayrik dal */
#define MODE_SPECIAL     1       /* +0x2a */
#define MODE_DEFAULT     64      /* +0x2a */
#define SUB_ARG          3       /* FUN_08014ffc ikinci argumani */

/* +0x4c'deki 12 baytlik ucluyu tek `ldmia`/`stmia` ciftiyle tasitmak icin
 * (kural 32). */
typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

/* Kardes dosyalarla (entries_a1.c, entries_a3.c, entries_a4.c,
 * kind_scan.c) ayni yerlesim; bu fonksiyonun dokundugu alanlar eklendi.
 * Toplam boyut 148 = 0x94. */
typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     pad01[3];
    u8     sub[38];             /* +0x04, FUN_08014ffc/FUN_08015038'e verilir */
    u8     mode;                /* +0x2a */
    u8     pad2b[33];
    Triple payload;             /* +0x4c */
    u8     pad58[12];
    u8     kind;                /* +0x64 */
    u8     pad65[3];
    u32    unk68;               /* +0x68 */
    u8     pad6c[24];
    u32    unk84;               /* +0x84 */
    u8     pad88[4];
    u32    unk8c;               /* +0x8c */
    u32    unk90;               /* +0x90, stride 148 */
} Entry;

extern Entry gEntriesA[];

extern void FUN_08014ffc(u8 *dest, u32 count, Triple *src, u32 *value);
extern void FUN_08015038(u8 *sub);
extern void FUN_080289bc(Entry *entry);

/* 0x08028A88 */
void FUN_08028a88(Triple *src, u32 arg1, u32 phase, u32 owner)
{
    Entry *e;
    s32 i;

    for (i = 0; i <= ENTRY_COUNT_MAX; i++) {
        if (gEntriesA[i].active == 0)
            break;
    }

    if (i > ENTRY_COUNT_MAX)
        return;

    e = &gEntriesA[i];
    e->active = 1;
    e->kind = WANTED_KIND;
    e->unk90 = phase;
    e->unk84 = owner;
    e->payload = *src;
    e->unk68 = arg1 << 16;
    e->unk8c = 0;

    FUN_08014ffc(e->sub, SUB_ARG, &e->payload, &e->unk68);

    if (phase == PHASE_SPECIAL)
        e->mode = MODE_SPECIAL;
    else
        e->mode = MODE_DEFAULT;

    FUN_08015038(e->sub);
    FUN_080289bc(e);
}
