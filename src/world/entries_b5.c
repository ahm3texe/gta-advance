/* FUN_08025518 — 0x08025518-0x080256E1 (458 bayt)
 *
 * Bir gEntriesA girisini turune (+0x64) ve evresine (+0x90) gore isliyor.
 * Uc is yapiyor:
 *   1. Tur 101 / 51 / 76 / 34 icin, evreye bakip iki sabitten birini
 *      (0x000E0000 ya da 0x00300000) secip FUN_08023df0 + FUN_08060db4
 *      ikilisine +0x4C'deki 12 baytlik ucluyle birlikte veriyor.  Dorduncu
 *      argumana gRom08CA61C0[e->owner] << 16 gidiyor.
 *   2. Tur 51/76 ya da evre 22/46 ise, ayni girisi sablon olarak verip
 *      CreateEntryFromTemplate (0x08025424, src/world/entries_b1.c) ile
 *      tur 34'ten yeni bir giris kuruyor; evre 47 + bildirim 461, ya da
 *      evre 7 + bildirim 258.  Ardindan gRam020245A0 = 1, gRam02024344 = 0.
 *   3. Evre 12 / 38 / 39 ise 1 dondurup girisi ayakta birakiyor; degilse
 *      FUN_08013abc(e->sub) ile alt nesneyi birakip +0x00'i sifirliyor ve
 *      0 donduruyor.
 *
 * IMZA: tek parametre (r0), epilog `pop {r1}; bx r1` — donus adresi r1'e
 * aliniyor, yani r0 canli, fonksiyon DEGER donduruyor (kural 35'in tersi).
 *
 * STRUCT ve CAGRI IMZALARI kardes dosyalardan alindi: Entry yerlesimi
 * src/world/entries_b1.c ve entries_a6.c ile ayni (148 bayt); +0x4C'deki
 * `Triple` src/world/submit_pack.c'deki `Pack12` ile ayni nesne
 * (FUN_08060db4 orada da (Triple *, u32) imzasiyla cagriliyor).
 *
 * ---------------------------------------------------------------------
 * ROM'DAN OLCULEN AYRINTILAR (hepsi diff ile dogrulandi)
 *
 *  1. DIS SWITCH gercekten `switch`.  0x8025528'de `cmp #76 / beq` hemen
 *     ardindan `cmp #76 / bgt` var: bu, agbcc'nin hem case degeri hem
 *     bolme noktasi olan bir dugum icin urettigi kalip (kural 46'nin
 *     agac tarafi).  `bgt` ISARETLI, cunku `ldrb` int'e yukseliyor.
 *     Case kumesi {34,51,76,101} seyrek, atlama tablosu cikmiyor.
 *     Case govdeleri KAYNAK SIRASINDA yayiliyor; ROM'daki sira
 *     101, 51/76, 34 -- dosyada da o sirada yazildi.
 *
 *  2. IC SWITCH (`case 34`) evreye gore: `cmp #28 / beq`, `cmp #28 / bhi`
 *     -> ISARETSIZ dal, yani `phase` u32 (kural 31).  Case 28'in govdesi
 *     bos.  Ic case'lerin KAYNAK SIRASI 28, 46, 22 olmali: govdesi en
 *     sonda kalan case'in `b` komutu silinip duse gecince o blok capraz
 *     atlama adayi OLMAKTAN CIKIYOR ve ayri fiziksel kopya olarak
 *     kaliyor.  ROM'da ayri kalan kopya 22'ninki (0x8025604), bu yuzden
 *     22 en sona yazildi.  22 basa alininca 46'ninki ayri kaliyor ve
 *     blok yerlesimi tumuyle kayiyor.
 *
 *  3. KURAL 44 BURADA BELIRLEYICI OLDU.  `if (e->phase == 46 ||
 *     e->phase == 47)` yazimini agbcc'nin `fold`'u ARALIK TESTINE
 *     ceviriyor: `subs r0,#46 / cmp r0,#1 / bhi`.  ROM iki ayri
 *     `cmp #46` / `cmp #47` kullaniyor.  47'yi yerele almak (`c47 = 47;`)
 *     katlamayi atlatiyor ve sabit yine immediate olarak yayiliyor.
 *     Yan etkisi cok daha buyuk: aralik testi yuzunden case 101'in
 *     govdesi otekilerle CAPRAZ ATLAMAYLA birlesiyordu; ayrilinca ROM'un
 *     UC ayri cagri bloguna kavusuluyor.  Tek satir: 414 -> 458 bayt.
 *     (case 51/76 dalinda ayni katlama OLMUYOR, cunku `||` zincirinin
 *     basinda `e->kind == 76` var ve fold ikili agacta bitisik iki
 *     esitligi goremiyor -- bu yuzden orada c47 gerekmiyor.)
 *
 *  4. KURAL 45 -- HER DALA AYRI YERELLER.  Dort cagri dalinin her biri
 *     kendi `p / w / tbl` uclusunu kullaniyor.  Paylasilan yereller
 *     hem bloklari birlestiriyor hem de DAGITIMI ceviriyor: paylasilan
 *     `w` global dagiticiya girip `e`'den sonra siraya giriyor ve r6'yi
 *     aliyor, `e` r5'te kaliyor -- ROM'un TERSI.  Dala ozel yereller
 *     dogrudan-akisli case govdelerinde tek bloga sigdigi icin YEREL
 *     dagiticiya dusuyor, r5'i erken kapiyor ve `e` r6'ya iniyor.
 *     Olculdu (hepsi 458 bayt, yalnizca fark sayisi):
 *         hepsi ayri            -> fark  0   (ESLESME)
 *         `tbl` ortak           -> fark  6
 *         `p`   ortak           -> fark 28
 *         `w`   ortak           -> fark 39
 *         yalnizca 101 ayri     -> fark 85   (e/r5, w/r6 ters)
 *         hicbiri ayri          -> 402 bayt (uc kopya ikiye iniyor)
 *
 *  5. ARGUMAN SIRASI: ROM once `gRom08CA61C0[owner] << 16`'yi, sonra
 *     &e->payload'i, en son e->unk84'u kuruyor.  Bu ancak tablo okumasi
 *     AYRI DEYIM olup `p = &e->payload;`den ONCE gelirse cikiyor.
 *     `FUN_08023df0(&e->payload, w, e->unk84, TABLE[owner] << 16)` tek
 *     satirda yazilirsa sira sagdan sola olur ve payload/unk84 yer
 *     degistirir.
 *
 *  6. KURAL 1 -- TABLO EXTERN SEMBOL OLMALI.  Sabit cast (`((u32 *)
 *     0x08CA61C0)[i]`) agbcc'ye ONCE indeksi hesaplatip havuz sabitini
 *     SONRA yukletiyor:
 *         ldrb / lsl / ldr =taban / add
 *     ROM ise tabani ONCE yukluyor:
 *         ldr =taban / ldrb / lsl / add
 *     Izole deneyle dogrulandi (dort yazim: sabit cast, ara isaretci
 *     yereli, tam sayi aritmetigi, dizi-isaretcisi cast -- DORDU DE
 *     yanlis sira).  Yalnizca `extern u32 gRom08CA61C0[];` dogru sirayi
 *     veriyor.  Uc blokta 9 komut, son 20 baytlik farkin tamami buydu.
 *
 *  7. KURAL 49 -- SEYREK GOVDE SONDA.  Kuyrukta ROM `return 1`'i akisin
 *     icinde, temizlik blogunu (`FUN_08013abc` + `+0x00 = 0`) en sonda
 *     tutuyor.  Duz `if (ok) return 1;` yazimi bunun TERSINI uretiyor
 *     (temizlik dusuyor, `return 1` sona atiliyor).  Uc cikisi da `goto
 *     keep;` ile ayni etikete yollayip etiketi temizlik blogundan ONCE
 *     koymak ROM'un sirasini veriyor: fark 47 -> 20.
 *
 *  8. KURAL 48 -- KOSULU DEGISKENDE MADDELESTIRME.  0x80256B4'te
 *     `movs r4,#0 / cmp #39 / bne / movs r4,#1 / cmp r4,#0 / beq`
 *     dizisi var; bu dogrudan dallanma degil, bayrak degiskeni.
 *     Ayrica sondaki `strb r4,[r6,#0]` o degiskenin sifir halini
 *     kullaniyor (cse dal kosulundan r4 == 0 oldugunu biliyor), yani
 *     kaynakta duz `e->active = 0;` yeterli.
 *
 * ---------------------------------------------------------------------
 * DENENIP ELENEN YAZIMLAR (ayni duvara toslamayin)
 *
 *  - `if (e->phase == 46 || e->phase == 47)` (c47 yereli olmadan):
 *    414 bayt.  Aralik testi hem 2 bayt kisaltiyor hem case 101'in
 *    cagri blogunu otekilerle birlestiriyor.  Sirayi cevirmek
 *    (`47 || 46`) de kurtarmaz, fold yine bitisik araligi gorur.
 *  - Ic switch'i `22, 28, 46` sirasiyla yazmak: 458 yerine 394 bayt.
 *    Sondaki case'in duse gecmesi belirleyici (madde 2).
 *  - Tablo tabanini yerele almak (`tab = (u32 *)0x08CA61C0; tab[i]`):
 *    kopya eleniyor, komut sirasi degismiyor (fark 47'de kaliyor).
 *    Ayni sonuc `u32 tab = 0x08CA61C0; *(u32 *)(tab + (i << 2))` ve
 *    `#define TABLE (*(u32 (*)[])0x08CA61C0)` yazimlarinda da cikti.
 *    Sabit yuklemesi her zaman kullanildigi yere ceziliyor.
 *  - Kuyrukta `if (ok == 0) goto cleanup; return 1; cleanup: ...`:
 *    blok sirasi DEGISMIYOR (fark 47).  Uc cikisin da ayni etikete
 *    gitmesi gerekiyor (madde 7).
 *  - `FUN_08023df0(&e->payload, w, e->unk84, TABLE[owner] << 16)` tek
 *    satirda: arguman kurma sirasi ters (madde 5).
 *
 * ---------------------------------------------------------------------
 * YENI SEMBOL: 0x08CA61C0 (gRom08CA61C0) data/ram_map.csv'de YOK ve bu
 * oturumda o dosyaya yazmak yasakti, bu yuzden adres dosya kapsamli
 * `asm(".equ ...")` ile veriliyor.  Bu GECICI bir kacamak; ram_map'e
 *     0x08CA61C0,0,gRom08CA61C0,decomp,provisional,
 *     "ROM: u32 tablo, e->owner (+0x01) ile indeksleniyor; deger <<16
 *      ile FUN_08023df0'a gidiyor (0x08025518)."
 * satiri eklenince asagidaki `asm` satiri silinip yalnizca `extern`
 * birakilmali.  Ayni kacamak src/core/nodelist_c3.c'de de kullanilmisti
 * (bkz. oradaki dosya basi notu).  Sabit cast KULLANILAMAZ: kural 1
 * geregi taban register'da tutulmuyor ve komut sirasi kayiyor (madde 6).
 *
 * `make c-review` bu iki satir yuzunden UYARI veriyor ("inline assembly"
 * + "ciplak adres"); ram_map satiri eklenip `asm` silinince denetim
 * TEMIZ oluyor.  Uretilen assembly'nin `asm` satiri OLMADAN da BIREBIR
 * ayni oldugu olculdu (tek fark tekrarlanan bir `.code 16` yonergesi,
 * bayta etkisi yok), yani eslesme o silme sonrasi da korunur.
 *
 * ESLESME: 458/458 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_b5.c
 */

#include "gba_types.h"

#define KIND_34    34           /* +0x64 */
#define KIND_51    51
#define KIND_76    76
#define KIND_101  101

#define PHASE_7      7          /* CreateEntryFromTemplate'e verilen evre */
#define PHASE_12    12
#define PHASE_22    22
#define PHASE_28    28
#define PHASE_38    38
#define PHASE_39    39
#define PHASE_46    46
#define PHASE_47    47

/* FUN_08023df0 / FUN_08060db4 ikinci argumani; ROM ikisini de
 * imm8 << n olarak kuruyor (0xE0<<12 ve 0xC0<<14). */
#define VALUE_A   0x000E0000
#define VALUE_B   0x00300000

#define NOTIFY_A   461          /* FUN_08035058 ikinci argumani */
#define NOTIFY_B   258

/* ROM tablosu; adresi data/ram_map.csv'de kayitli, derleme katmani
 * (tools/agbcc_build.py) `.equ` bildirimini KENDISI uretiyor -- bu yuzden
 * burada inline assembly YOK.  Dizi bildirimi zorunlu: baska yazimlar
 * arguman sirasini bozuyor (bkz. dosya basindaki elenen yollar). */
extern u32 gRom08CA61C0[];

/* +0x4C'deki 12 baytlik uclu; src/world/entries_b1.c'deki `Triple` ve
 * src/world/submit_pack.c'deki `Pack12` ile ayni nesne. */
typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

/* Kardes dosyalarla (entries_a5.c, entries_a6.c, entries_b1.c) ayni
 * yerlesim; bu fonksiyonun dokundugu alanlar acildi.  Toplam 148 = 0x94. */
typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     owner;               /* +0x01, ROM tablosuna indeks olarak da kullaniliyor */
    u8     pad02[2];
    u8     sub[38];             /* +0x04, FUN_08013abc'ye verilir */
    u8     pad2a[34];
    Triple payload;             /* +0x4C */
    u8     pad58[12];
    u8     kind;                /* +0x64 */
    u8     pad65[31];
    u32    unk84;               /* +0x84 */
    u8     pad88[8];
    u32    phase;               /* +0x90, stride 148 */
} Entry;

extern u8  gRam020245A0;
extern u16 gRam02024344;

extern void FUN_08023df0(Triple *payload, u32 value, u32 arg2, u32 arg3);
extern void FUN_08060db4(Triple *payload, u32 value);
extern void FUN_08013abc(u8 *sub);
extern u32  GetActiveSlotValue(void);
extern void FUN_08035058(u32 slot, u32 id);
extern u32  CreateEntryFromTemplate(Entry *src, u32 unused1, u32 unused2,
                                    u8 kind, u32 phase, u8 owner);

/* 0x08025518 */
u32 FUN_08025518(Entry *e)
{
    /* Kural 45: her cagri dalinin KENDI uclusu var.  Bildirim sirasi
     * degistirilmemeli, dagitim sirasini belirliyor (bkz. dosya basi 4). */
    Triple *p76;
    u32     w76;
    u32     tbl76;
    u32     ok;
    Triple *p101;
    u32     w101;
    u32     tbl101;
    u32     c47;                /* kural 44: aralik testini engelleyen yerel */
    Triple *p46;
    u32     w46;
    u32     tbl46;
    Triple *p22;
    u32     w22;
    u32     tbl22;

    switch (e->kind) {
    case KIND_101:
        c47 = PHASE_47;
        if (e->phase == PHASE_46 || e->phase == c47)
            w101 = VALUE_A;
        else
            w101 = VALUE_B;
        tbl101 = gRom08CA61C0[e->owner] << 16;
        p101 = &e->payload;
        FUN_08023df0(p101, w101, e->unk84, tbl101);
        FUN_08060db4(p101, w101);
        break;
    case KIND_51:
    case KIND_76:
        if (e->kind == KIND_76 || e->phase == PHASE_46 || e->phase == PHASE_47)
            w76 = VALUE_A;
        else
            w76 = VALUE_B;
        tbl76 = gRom08CA61C0[e->owner] << 16;
        p76 = &e->payload;
        FUN_08023df0(p76, w76, e->unk84, tbl76);
        FUN_08060db4(p76, w76);
        break;
    case KIND_34:
        switch (e->phase) {
        case PHASE_28:
            break;
        case PHASE_46:
            w46 = VALUE_A;
            tbl46 = gRom08CA61C0[e->owner] << 16;
            p46 = &e->payload;
            FUN_08023df0(p46, w46, e->unk84, tbl46);
            FUN_08060db4(p46, w46);
            break;
        case PHASE_22:          /* en sonda kalmali, bkz. dosya basi 2 */
            w22 = VALUE_B;
            tbl22 = gRom08CA61C0[e->owner] << 16;
            p22 = &e->payload;
            FUN_08023df0(p22, w22, e->unk84, tbl22);
            FUN_08060db4(p22, w22);
            break;
        }
        break;
    }

    if (e->kind == KIND_51 || e->kind == KIND_76
        || e->phase == PHASE_22 || e->phase == PHASE_46) {
        if (e->kind == KIND_76 || e->phase == PHASE_46) {
            CreateEntryFromTemplate(e, 1, 0, KIND_34, PHASE_47, e->owner);
            FUN_08035058(GetActiveSlotValue(), NOTIFY_A);
        } else {
            CreateEntryFromTemplate(e, 1, 0, KIND_34, PHASE_7, e->owner);
            FUN_08035058(GetActiveSlotValue(), NOTIFY_B);
        }
        gRam020245A0 = 1;
        gRam02024344 = 0;
    }

    /* Kural 49: temizlik blogu SONDA; uc cikis da ayni etikete gidiyor. */
    if (e->phase == PHASE_12)
        goto keep;
    if (e->phase == PHASE_38)
        goto keep;
    ok = 0;                     /* kural 48: kosul degiskende maddelesiyor */
    if (e->phase == PHASE_39)
        ok = 1;
    if (ok == 0)
        goto retire;
keep:
    return 1;
retire:
    FUN_08013abc(e->sub);
    e->active = 0;
    return 0;
}
