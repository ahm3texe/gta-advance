/* gEntriesA'da bos yuva acip sablondan yeni giris kurma — 0x08025424-0x08025515
 *
 * Kardesi src/world/entries_a5.c (0x08028A88) ile ayni ailedendir: ikisi de
 * gEntriesA tablosunda (15 giris, 148 bayt stride) +0x00 alani sifir olan
 * ilk bos yuvayi arar. Bu surum farkli olarak
 *   - yuvayi ELDEKI BIR GIRISTEN (src) kopyalayarak doldurur,
 *   - ROM tablosundan (gRom08BD3448) uc asamali bir cozumleme yapip
 *     elde ettigi tanimlayiciyi alt nesneye baglar,
 *   - bos yuva bulamazsa 0, kurarsa 1 dondurur.
 *
 * Akis:
 *   desc = gRom08BD3448.slots[kind]->slots[phase]->slots[0]
 *   bos yuva yoksa 0 don
 *   e->kind/phase/+0x66 kurulur; src'den +0x4C ucusu, +0x68 ve +0x84 kopyalanir
 *   FUN_08014FFC / FUN_08013CFC / FUN_08014EE4 / FUN_08015038 alt nesneyi kurar
 *   e->active=1, +0x8C=0, timer=125, owner=arg5
 *   phase 47 ise mod 64, degilse 16
 *
 * IMZA cagri yerinden dogrulandi (0x08025662 ve 0x0802568C): alti arguman
 * veriliyor (r0, 1, 0, 34, [sp,#0], [sp,#4]). Ikinci ve ucuncu arguman
 * cagrilanda HIC OKUNMUYOR; ROM daha girerken `adds r2,r0,#0` ile r0'i r2'ye
 * tasiyip r1/r2'yi eziyor. Bu yuzden imzada duruyorlar ama kullanilmiyorlar.
 *
 * PARAMETRE TIPLERI komut dizisinden okundu (kural 15): dorduncu ve altinci
 * arguman `lsls #24 / lsrs #24` ciftiyle daraltiliyor -> ISARETSIZ 8 bit;
 * besinci arguman ham kelime olarak kullaniliyor -> u32.
 *
 * Kural 35'in tersi: epilog `pop {r1}; bx r1` — donus adresi r1'e aliniyor,
 * yani r0 canli, fonksiyon DEGER donduruyor (u32).
 *
 * Kural 32: +0x4C'deki 12 bayt `ldmia r0!,{r3,r4,r7}` / `stmia r1!,{...}`
 * ciftiyle tasiniyor; bu ancak struct atamasindan (Triple) cikiyor,
 * `*d++ = *s++` ucluleri uc ayri ldr/str veriyor.
 *
 * Kural 9/31: dongu ici dal `cmp #14` + `bgt`, yani ISARETLI -> sayac `s32`.
 * Dongu sonrasi dal ise `cmp #15` + `bne`; bu `i > 14` degil `i == 15`
 * yaziminin karsiligidir (`i > 14` yazilinca agbcc yine `cmp #14`/`bgt`
 * uretiyor ve blok sirasi kayiyor).
 *
 * Kural 19: yuva doldurma sirasinda kaynak sirasi ROM'un `adds`/`subs`
 * zincirini belirliyor: +0x64 (adds #100), +0x90 (adds #44), +0x66
 * (subs #42). Sira degistirilirse ofset zinciri de degisiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_b1.c
 */

#include "gba_types.h"

#define ENTRY_COUNT     15      /* gEntriesA'daki yuva sayisi */
#define SUB_ARG         131     /* FUN_08014FFC ikinci argumani (0x83) */
#define TIMER_START     125     /* +0x02 baslangic degeri */
#define PHASE_SPECIAL   47      /* +0x90 == 47 ayrik dal */
#define MODE_SPECIAL    64      /* +0x2A */
#define MODE_DEFAULT    16      /* +0x2A */

/* +0x4C'deki 12 baytlik ucluyu tek ldmia/stmia ciftiyle tasitmak icin
 * (kural 32); entries_a5.c'deki Triple ile ayni. */
typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

/* 0x08BD3448'deki ROM koku ve ondan zincirlenen dugumler. Kok ve ara
 * dugumler AYNI seklde: +0x04'te bir isaretci dizisine isaretci. Bu yuzden
 * tek ozyinelemeli tip yeterli. Ayni sembol src/text/glyph_table_access.c'de
 * DAHA SIG bir gorunumle (GlyphRoot/GlyphEntry) bildirilmis; oradaki +0x14
 * alani burada da okunuyor, yani ayni fiziksel dugum. Bu ceviri biriminin
 * ihtiyaci olan derinlik farkli oldugu icin gorunum ayri tutuldu. */
typedef struct RomNode {
    u32             pad00;
    struct RomNode **slots;     /* +0x04 */
    u8              pad08[12];
    u32             unk14;      /* +0x14 */
} RomNode;

extern RomNode gRom08BD3448;

/* Kardes dosyalarla (entries_a1.c ... entries_a5.c, kind_scan.c) ayni
 * yerlesim; bu fonksiyonun dokundugu alanlar acildi. Toplam 148 = 0x94. */
typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     owner;               /* +0x01 */
    u16    timer;               /* +0x02 */
    u8     sub[38];             /* +0x04, FUN_08014FFC/FUN_08015038'e verilir */
    u8     mode;                /* +0x2A */
    u8     pad2b[33];
    Triple payload;             /* +0x4C */
    u8     pad58[12];
    u8     kind;                /* +0x64 */
    u8     pad65;
    u16    unk66;               /* +0x66 */
    u32    unk68;               /* +0x68 */
    u8     pad6c[24];
    u32    unk84;               /* +0x84 */
    u8     pad88[4];
    u32    unk8c;               /* +0x8C */
    u32    phase;               /* +0x90, stride 148 */
} Entry;

extern Entry gEntriesA[];

extern void FUN_08014ffc(u8 *dest, u32 count, Triple *src, u32 *value);
extern void FUN_08013cfc(u8 *dest, RomNode *desc, u32 arg);
extern void FUN_08014ee4(u8 *dest, u32 value);
extern void FUN_08015038(u8 *dest);

/* 0x08025424 */
u32 FUN_08025424(Entry *src, u32 unused1, u32 unused2, u8 kind, u32 phase,
                 u8 owner)
{
    Entry *e;
    RomNode *desc;
    s32 i;

    desc = gRom08BD3448.slots[kind]->slots[phase]->slots[0];

    for (i = 0; i < ENTRY_COUNT; i++) {
        if (gEntriesA[i].active == 0)
            break;
    }

    if (i == ENTRY_COUNT)
        return 0;

    e = &gEntriesA[i];
    e->kind = kind;
    e->phase = phase;
    e->unk66 = 0;
    e->payload = src->payload;
    e->unk68 = src->unk68;
    e->unk84 = src->unk84;

    FUN_08014ffc(e->sub, SUB_ARG, &e->payload, &e->unk68);
    FUN_08013cfc(e->sub, desc, 0);
    FUN_08014ee4(e->sub, desc->unk14);
    FUN_08015038(e->sub);

    e->active = 1;
    e->unk8c = 0;
    e->timer = TIMER_START;
    e->owner = owner;

    if (phase == PHASE_SPECIAL)
        e->mode = MODE_SPECIAL;
    else
        e->mode = MODE_DEFAULT;

    return 1;
}
