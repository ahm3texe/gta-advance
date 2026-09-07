/* gRam020245B0 tablosunda bos yuva acip yeni giris kurma — 0x08028B9C-0x08028C47
 *
 * Kardesi src/world/entries_a5.c (0x08028A88) ve src/world/entries_b1.c
 * (0x08025424) ile ayni ailedendir: 148 bayt stride'li bir tabloda +0x00
 * alani sifir olan ilk bos yuvayi arar, bulursa doldurup alt nesneyi
 * (sub, +0x04) kurar. Farklari:
 *   - tablo gEntriesA degil, TEK GIRISLIK gRam020245B0 (data/ram_map.csv:74),
 *   - tanimlayici ROM kokunden SABIT indislerle cozuluyor:
 *       desc = gRom08BD3448.slots[64]->slots[0]->slots[0]
 *   - yuvanin +0x68 ve +0x4C alanlari cagiranin verdigi nesneden geliyor,
 *   - bos yuva bulunamazsa 0xFF, kurulursa YUVA INDISI donuyor.
 *
 * Akis:
 *   desc  = gRom08BD3448.slots[64]->slots[0]->slots[0]
 *   bos yuva yoksa 0xFF don
 *   e->unk68   = h->unk18->unk0c
 *   e->payload = h->unk14->items[index]      (12 baytlik uclu)
 *   FUN_08014FFC(e->sub, 7, &e->payload, &e->unk68)
 *   FUN_08013CFC(e->sub, desc, arg2)
 *   FUN_08014EE4(e->sub, desc->unk14)
 *   FUN_08015038(e->sub)
 *   e->active = 1;  return i;
 *
 * ROM'DAN OLCULEN AYRINTILAR
 * --------------------------
 * IMZA cagri yerinden DOGRULANAMADI: ROM'da 0x08028B9C'ye giden tek bir BL
 * yok ve fonksiyonun adresi (0x08028B9C / 0x08028B9D) hicbir veri
 * kelimesinde gecmiyor -- yani cagri, taranmamis bir tablodan dolayli
 * yapiliyor. Bu yuzden arity yalnizca GOVDEDEN okundu: r3 kullaniliyor,
 * dolayisiyla en az dort parametre var; r1 hic okunmadan 0x8028BB2'de
 * `movs r1,#128` ile eziliyor, yani IKINCI ARGUMAN KULLANILMIYOR.
 * entries_b1.c'de de iki olu argumanin bulunmasi bu ailede bunun olagan
 * oldugunu gosteriyor.
 *
 * PARAMETRE TIPLERI (kural 15): ucuncu ve dorduncu arguman giriste
 * `lsls #24 / lsrs #24` ciftiyle daraltiliyor -> ISARETSIZ 8 bit.
 * Birinci arguman ham isaretci, ikinci arguman olu (u32 birakildi).
 *
 * Kural 35'in tersi: epilog `pop {r1}; bx r1` -- donus adresi r1'e aliniyor
 * cunku r0 canli. Fonksiyon DEGER donduruyor. Basari yolunda `adds r0,r6,#0`
 * (dongu sayaci), hata yolunda `movs r0,#255`. Sayac s32 oldugu icin donus
 * tipi u32 birakildi; u8 yapmak donuse daraltma komutu ekletme riski tasir.
 *
 * DONGU BICIMI ROM'DAN OKUNDU, KARDESTEN KOPYALANMADI:
 *   0x8028BD4  cmp r6,#0 / bgt   -> dongu kosulu `i < 1` (agbcc `<= 0`ya
 *                                   kanonikleştiriyor, kural 44'un tersi yonu)
 *   0x8028BDE  cmp r6,#1 / beq   -> dongu sonrasi kontrol `i == 1`
 * Yani sinir 1'dir; ram_map'teki "1 x 148 giris" notuyla birebir tutuyor.
 * Tek iterasyonluk olmasina ragmen agbcc dongu dondurmesini (rotation)
 * yapiyor: giris testi eleniyor, ilk `ldrb` govde disinda kaliyor, geri
 * kalani do/while oluyor.
 *
 * Kural 49: hata govdesi (`movs r0,#255`) fonksiyonun SONUNDA, literal
 * havuzdan sonra duruyor. Duz `if (i == ENTRY_COUNT) return FAIL;` yazimi
 * bunu kendiliginden veriyor, etiket gerekmedi.
 *
 * Kural 32: +0x4C'deki 12 bayt `ldmia r1!,{r2,r3,r4}` / `stmia r0!,{...}`
 * ciftiyle tasiniyor; bu ancak struct atamasindan (Triple) cikiyor.
 * `ldmia` r4'u (birinci arguman) eziyor, yani cagiranin nesnesi bu
 * noktadan sonra olu -- kaynakta da ondan sonra kullanilmiyor.
 *
 * KAYNAK INDISININ SEKILLENMESI (kural 28): 12 baytlik ogenin adresi
 * `lsls #1 / adds / lsls #2` ile ONCE indis*12 hesaplanip tabana ekleniyor,
 * sonra `adds r1,#116` geliyor. Bu, dizi indisi biciminin (`p->items[i]`)
 * imzasidir; her terimi ayri olcekleyen isaretci aritmetigi degil.
 *
 * SABIT INDISLI ROM ZINCIRI: 64 indisi word olarak 0x100 ediyor, Thumb
 * `ldr` immediate araligi disinda, bu yuzden `movs r1,#128 / lsls r1,#1 /
 * adds r0,r0,r1 / ldr r0,[r0]` cikiyor. Sonraki iki asama ofset 0 oldugu
 * icin `ldr r0,[r0,#4] / ldr r0,[r0,#0]` ciftlerine iniyor.
 *
 * DENENIP ELENENLER (hepsi bu dosyada olculdu, asagidaki yazim eslesirken)
 * ----------------------------------------------------------------------
 * - Dongu sonrasi kontrolu `if (i > 0)` yazmak: 3/172 fark. agbcc bunu
 *   dongunun kendi `cmp #0`/`bgt` testiyle ayni kanonik bicime sokup
 *   birlestiriyor; ROM'da o noktada AYRI bir `cmp #1`/`beq` var. Kural 44'un
 *   ayni ailesi: sinir degeriyle yazilan esitlik testi kanonikleştirmeden
 *   kurtuluyor. entries_b1.c'de ayni sey `i == 15` ile cozulmustu.
 * - Dongu kosulunu `i <= 0` yazmak: ESLESIYOR, `i < ENTRY_COUNT` ile bire
 *   bir ayni kod. Yani kanonikleştirme dongu testinde gercekten oluyor;
 *   ayrimi yapan tek yer dongu SONRASI kontrol. Okunurluk icin `<` biraktim.
 * - Donus tipini `u8` yapmak: 176 bayt, 29 fark. agbcc her `return` yolunda
 *   `lsls #24 / lsrs #24` daraltmasi ekliyor; ROM'da yok. Sayac s32 olarak
 *   ham donuyor, o yuzden imza u32.
 * - Dorduncu argumani `u32 index` yapmak: 168 bayt (4 kisa). Giristeki
 *   `lsls r3,#24 / lsrs r3,#24` cifti kayboluyor -- kural 15'in dogrudan
 *   kaniti, arguman gercekten u8.
 * - Ucluyu `*(h->unk14->items + index)` ile almak: 6 fark. Kural 28'in
 *   karsi ornegi: isaretci biciminde `adds #116` ofseti carpimdan ONCE
 *   yayiliyor, ROM'da SONRA geliyor. Dizi indisi bicimi gerekiyor.
 * - Iki atamanin sirasini ters cevirmek (once payload, sonra unk68):
 *   182 bayt, 166 fark. Kural 19 bu fonksiyonda en sert etkiyi burada
 *   gosterdi: sira degisince ldmia'nin ezdigi r4 hala canli kaliyor,
 *   dagitim tumuyle kayiyor ve fazladan bir yigin gozu aciliyor.
 * - Olu ikinci argumani imzadan cikarmak (uc parametre): 5 fark. index r3
 *   yerine r2'ye dusuyor ve giristeki daraltma sirasi bozuluyor -- olu
 *   arguman gercekten imzada.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_a7.c
 */

#include "gba_types.h"

#define ENTRY_COUNT   1     /* gRam020245B0'daki yuva sayisi */
#define FAIL_INDEX    255   /* bos yuva yoksa donen deger */
#define ROOT_SLOT     64    /* gRom08BD3448.slots[] icindeki sabit indis */
#define SUB_ARG       7     /* FUN_08014FFC ikinci argumani */

/* +0x4C'deki 12 baytlik ucluyu tek ldmia/stmia ciftiyle tasitmak icin
 * (kural 32); entries_a5.c / entries_b1.c'deki Triple ile ayni. */
typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

/* 0x08BD3448'deki ROM koku ve ondan zincirlenen dugumler; src/world/
 * entries_b1.c ve src/world/entries_a6.c'deki RomNode ile ayni yerlesim.
 * Bu ceviri birimi yalnizca +0x04 (alt dugum tablosu) ve +0x14 alanlarini
 * okuyor. */
typedef struct RomNode {
    u32             pad00;
    struct RomNode **slots;     /* +0x04 */
    u8              pad08[12];
    u32             unk14;      /* +0x14 */
} RomNode;

extern RomNode gRom08BD3448;

/* Cagiranin verdigi nesnenin +0x18'inde duran kayit; yalnizca +0x0C
 * okunuyor, digerleri dolgu. */
typedef struct SourceState {
    u8  pad00[12];
    u32 unk0c;                  /* +0x0C -> yuvanin +0x68'i */
} SourceState;

/* Cagiranin verdigi nesnenin +0x14'unde duran kayit; +0x74'ten itibaren
 * 12 baytlik ogelerden olusan bir dizi tutuyor (stride ROM'da lsl/add/lsl
 * ile 12 olarak olculdu). Dizinin uzunlugu bilinmiyor, esnek birakildi. */
typedef struct SourceTable {
    u8     pad00[116];
    Triple items[1];            /* +0x74 */
} SourceTable;

/* Fonksiyona verilen nesne. Yalnizca iki isaretci alani okunuyor; bu
 * ofsetler (0x14 / 0x18) alt nesne (sub) bolgesiyle ortusuyor ama bu
 * ceviri biriminde daha fazlasi kanitlanamadigi icin isim verilmedi. */
typedef struct Source {
    u8           pad00[20];
    SourceTable *unk14;         /* +0x14 */
    SourceState *unk18;         /* +0x18 */
} Source;

/* 0x020245B0'daki tek girislik tablo (data/ram_map.csv: 148 bayt).
 * Yerlesim kardes dosyalardaki Entry ile ayni; burada yalnizca dokunulan
 * alanlar acildi. Toplam 148 = 0x94. */
typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     pad01[3];
    u8     sub[38];             /* +0x04, FUN_08014FFC/FUN_08015038'e verilir */
    u8     pad2a[34];
    Triple payload;             /* +0x4C */
    u8     pad58[16];
    u32    unk68;               /* +0x68 */
    u8     pad6c[40];           /* stride 148 */
} Entry;

extern Entry gRam020245B0[];

extern void FUN_08014ffc(u8 *dest, u32 count, Triple *src, u32 *value);
extern void FUN_08013cfc(u8 *dest, RomNode *desc, u32 arg);
extern void FUN_08014ee4(u8 *dest, u32 value);
extern void FUN_08015038(u8 *dest);

/* 0x08028B9C */
u32 SpawnEntryFromTable(Source *h, u32 unused1, u8 arg2, u8 index)
{
    Entry *e;
    RomNode *desc;
    s32 i;

    desc = gRom08BD3448.slots[ROOT_SLOT]->slots[0]->slots[0];

    for (i = 0; i < ENTRY_COUNT; i++) {
        if (gRam020245B0[i].active == 0)
            break;
    }

    if (i == ENTRY_COUNT)
        return FAIL_INDEX;

    e = &gRam020245B0[i];
    e->unk68 = h->unk18->unk0c;
    e->payload = h->unk14->items[index];

    FUN_08014ffc(e->sub, SUB_ARG, &e->payload, &e->unk68);
    FUN_08013cfc(e->sub, desc, arg2);
    FUN_08014ee4(e->sub, desc->unk14);
    FUN_08015038(e->sub);

    e->active = 1;
    return i;
}
