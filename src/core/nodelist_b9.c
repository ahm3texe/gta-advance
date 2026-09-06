/* Bekleyen alan adindan oturumu yeniden kurma -- 0x08053B44, 516 bayt.
 *
 * gUnk02010C60 oturum blogundaki "bekleyen ad" alanindan (+0x08) hangi
 * kayit grubunun yuklenecegi bulunuyor, geri donus yigini (+0x10) bir
 * kademe soyuluyor, sonra alan baglami bastan kuruluyor:
 *
 *   1. Yigina iki adet 3'luk ad tablosu KOPYALANIYOR (ldmia/stmia ciftleri):
 *        gEmptyAreaNames   0x083E3004 -> {"empty1","empty2","empty3"}
 *        gSpecialAreaNames 0x083E3010 -> {"special1","special2","special3"}
 *      Ikisi de yerel; ROM sp+0 ve sp+12'ye yaziyor, sp+12'nin adresini
 *      r9'da tutuyor.
 *   2. Bekleyen ad once KAYIT GRUPLARININ ikincil kimlik dizisinde
 *      (gAreaBank.groups[i].idsB) aranıyor. Bulunursa grup indeksi,
 *      bulunmazsa ID_NONE.  Karsilastirici FUN_0806dd18 = strcmp
 *      (0x0806DD18 govdesi klasik kelime hizali strcmp).
 *   3. Yigin derinligi 0 degilse ve secilen grup yiginin TEPESIYSE yigin
 *      sifirlaniyor; hala doluysa ad yigindan (empty/special tablosundan)
 *      yeniden aliniyor ve indeks derinlik-1 oluyor.
 *   4. Ad bu kez BUTUN ad tablosunda (gAreaBank.names, 228 giris) aranıyor.
 *   5. gAreaNames[nameIdx].flags bit 3 ya da oturumun +0x1C bayragi ->
 *      gRam020004A0 = 1, ikisi de yoksa 0.
 *   6. Kayit, serbest listeler ve alan baglami yeniden kuruluyor.
 *
 * ROM'DAN OKUNAN YAPI IPUCLARI:
 *   - gAreaBank (0x08D49C00) bu dosyada dort alaniyla goruluyor:
 *       +0x08 nameCount = 228, +0x14 groupCount = 3,
 *       +0x20 names = 0x08D482E0, +0x2C groups = 0x08D49BD0.
 *     +0x24 (entries) src/core/nodelist_d5.c'de, +0x08/+0x20
 *     src/core/nodelist_b2.c'de zaten olculmustu; burada +0x14 eklendi.
 *   - AreaGroup (16 bayt) src/core/nodelist_d5.c'dekinin genisi:
 *     +0x00 ad ("gta_script_island1..3"), +0x04 birincil sayac,
 *     +0x05 IKINCIL sayac, +0x08 birincil kimlik dizisi,
 *     +0x0C IKINCIL kimlik dizisi.  Bu fonksiyon YALNIZCA ikincil ciftini
 *     kullaniyor (+0x05 / +0x0C) -- kardes dosyanin +0x04/+0x08'i degil.
 *   - AreaName 28 bayt, +0x00 ad isaretcisi (nodelist_b2.c ile ayni),
 *     +0x1A bayrak bayti.  ROM'daki 228 girisin 28'inde bit 3 kurulu.
 *
 * OLCULEN ALTI AYRINTI (her biri tek basina denendi, altisi da belirleyici):
 *
 * 1) IKI AD TABLOSU AYRI SEMBOL OLMALI, ciplak adres OLMAZ.
 *    `*(const NameTable *)0x083E3010` yazildiginda CSE, birinci
 *    `ldmia r0!` sonrasinda r0'in ZATEN 0x083E3004+12 = 0x083E3010
 *    oldugunu goruyor ve ikinci havuz yuklemesini tumden atiyor:
 *    508 bayt (2 bayt komut + 4 bayt havuz + 2 bayt hizalama = 8 eksik).
 *    Iki AYRI extern sembolde CSE sembol aritmetigini bilemedigi icin
 *    ROM'un iki `ldr r0,havuz` komutu geri geliyor.  Bu fonksiyonun
 *    ciplak adresle ESLESMESI MUMKUN DEGIL.
 *
 * 2) gAreaNames DIZI olarak bildirilmeli, isaretci sabiti olarak degil.
 *    `((const AreaName *)ADRES)[nameIdx].flags` havuz yuklemesini
 *    olcekleme SONRASINA koyuyor:
 *        lsls/subs/lsls, ldr rB,havuz, adds
 *    ROM ise once tabani yukluyor:
 *        ldr r2,havuz, lsls/subs/lsls, adds r0,r0,r2
 *    `extern const AreaName gAreaNames[];` (gercek ARRAY_REF) ROM'un
 *    sirasini veriyor.  Ayni zamanda nameIdx'in yazmacini r0'dan
 *    ROM'un r1'ine tasiyor -- tek degisiklik iki farki birden kapatiyor.
 *
 * 3) ARAMA SONUCU ILE YIGIN INDEKSI AYRI IKI YEREL, KOPYA YONU ONEMLI.
 *    ROM `adds r1,r7,#0` uretiyor: aranan deger r7'de dogup r1'e
 *    KOPYALANIYOR; sonra r7 `slot = depth - 1` ile EZILIYOR, r1 ise
 *    eski degeri tasiyip 0x08053C44'teki `sel + 1 != depth` testinde
 *    kullaniliyor.  Yani arama sonucu `slot`a yazilip `sel = slot;`
 *    denmeli.  Ters yazim (`sel`e arayip `slot = sel;`) kopyayi ters
 *    cevirip iki yazmaci takasliyor.  Kopya burada ELENMIYOR cunku iki
 *    degiskenin SON degerleri farkli -- memory'deki "kopya tabanli bolme
 *    her zaman elenir" kurali yalnizca ikisi de ayni kalirken gecerli.
 *
 * 4) IKINCI ARAMANIN SONUCU icin `int raw` + `u16` cifti (kural 15/35
 *    ailesi, src/core/nodelist_b2.c ile ayni kalip).  `u16 nameIdx`e
 *    DOGRUDAN yazmak daraltmayi her dalda ureten `lsls/lsrs` cikartiyor;
 *    ROM daraltmayi dallarin BIRLESTIGI yerde bir kez yapiyor.
 *    `rawName` (int) dallarda, `nameIdx = rawName;` birlesme noktasinda.
 *    Birinci aramada boyle bir daraltma YOK -- orada tek `int` yeterli.
 *
 * 5) BIRINCI DONGUDE ADAY ICIN AYRI ISARETCI YERELI (kural 4 / d5 madde 4).
 *    `gAreaBank.names[...].name` duz yazildiginda `ldr rX,[r6,#32]`
 *    ldrh'den ONCE cikiyor; ROM once kimligi okuyup id*28'i hesapliyor,
 *    `names` uyesini SONRA yukluyor ve toplami TABAN yazmacinda birakiyor.
 *    `const AreaName *cand = &gAreaBank.names[...];` bu sirayi veriyor.
 *    IKINCI donguda tersi: orada ROM zaten uyeyi once yukluyor, ayri
 *    yerel GEREKMIYOR -- kardes dosyalardaki gibi ezberlenmez, olculur.
 *
 * 6) IKI SIFIR ATAMASI ZINCIRLI: `gRam02026F34 = gRam020272C8 = 0;`.
 *    ROM iki adresi de store'lardan ONCE yukluyor, sonra ters sirada
 *    saklıyor (`ldr r1,=..2026F34 / ldr r0,=..20272C8 / str [r0] / str [r1]`).
 *    Bu tam olarak zincirli atamanin expand sirasi: dis atamanin HEDEF
 *    adresi once cozuluyor, sonra ic atama (adres + deger + store)
 *    calisiyor, en sonda dis store ediliyor.  Ayri iki deyim yazmak her
 *    store'un onune kendi havuz yuklemesini koyuyor.
 *
 * 7) KAYIT ISARETCISI AYRI YERELE ALINMALI.
 *    `gRam020357E0 = GetRecord(slot)->f20;` yazildiginda GCC once SOL
 *    tarafin adresini cozuyor; sembol yuklemesi `bl GetRecord`in USTUNE
 *    cikiyor ve callee-saved r4'u tutuyor.  ROM adresi cagridan SONRA
 *    yukluyor (r1).  `rec = GetRecord(slot); gRam020357E0 = rec->f20;`
 *    sagi once degerlendirtiyor ve ROM'un sirasini veriyor.
 *
 * ORTA BLOK (0x08053C06-0x08053C5A) ILK YAZIMDA BIREBIR TUTTU:
 *   `sel + 1 == depth` -> yigin tepesindeysek yigini sifirla; yigin hala
 *   doluysa `slot = depth - 1` ve ad empty tablosundan; sonra special
 *   bayragi varsa ad special tablosundan; bayrak yoksa ve yigin hala
 *   doluysa yine empty tablosundan.  Son iki dal ROM'da ortak kuyruga
 *   (ldr + mov r8) kavusturulmus durumda; kaynakta iki ayri atama olarak
 *   yazmak yeterli, derleyici kuyrugu kendisi birlestiriyor.
 *
 * DIGER UYGULANAN KURALLAR:
 *   - Kural 1: gAreaBank / gAreaNames / oturum blogu extern sembol.
 *   - Kural 9/31: butun sayaclar `int`; ROM `blt`/`bge` (isaretli).
 *   - Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *   - Kural 45: her donguye kendi sayaci (i, j, k); paylasilan sayac
 *     birinci donguyu ikincinin dagitimina baglıyor.
 *   - Kural 49: seyrek dallar (ID_NONE atamalari) ROM'da oldugu gibi
 *     `goto found` ile kendi bloklarina birakildi.
 *
 * DENENIP ELENENLER (tekrar denemeyin):
 *   - `#define` ile ciplak adres (uc ROM tablosu icin): madde 1, 508 bayt.
 *   - `entry = &gAreaNames[nameIdx];` ayri isaretci yereli: madde 2'deki
 *     yanlis sirayi DEGISTIRMIYOR; belirleyici olan bildirimin DIZI olmasi.
 *   - `u16 nameIdx`e dogrudan atama: madde 4.
 *   - `sel` arayip `slot = sel;`: madde 3, iki yazmac takasliyor.
 *   - Iki sifir atamasini ayri deyim yazmak: madde 6.
 *   - `gRam020357E0 = GetRecord(slot)->f20;` tek satir: madde 7.
 *
 * data/ram_map.csv'ye GEREKEN KAYITLAR (bu dosya onlarsiz DERLENMEZ):
 *   0x083E3004,12,gEmptyAreaNames     ROM: 3 x char*  {"empty1".."empty3"}
 *   0x083E3010,12,gSpecialAreaNames   ROM: 3 x char*  {"special1".."special3"}
 *   0x08D482E0,6384,gAreaNames        ROM: 228 x 28 baytlik AreaName;
 *                                     gAreaBank.names (+0x20) ile ayni deger
 *   0x020357E0,4,gRam020357E0         GetRecord(slot)->f20 buraya yaziliyor
 *   0x02035760,0,gRam02035760         FUN_08053834'un ILK argumani (AreaCtx);
 *                                     boyut BILINMIYOR, yalnizca tabani olculdu
 *
 * DOGRULAMA (kayitlar eklenmeden once yapilan olcum):
 *   Bes sembolun yerine ram_map'te ZATEN kayitli, ayni tipte bes sembol
 *   konuldu (gRom08BD3448 / gRom08CA61C0 / gRom08852A2C / gRam02001450 /
 *   gRam020110C0).  Sonuc 516/516 bayt ve TEK bir komut bile sapmadi;
 *   fark yalnizca degistirilen bes havuz SABITINDE kaldi.  Gercek
 *   adresler kayit edilince eslesme tamdir; sembol aritmetigi derleme
 *   zamaninda gorunmedigi icin adres degeri kod uretimini etkilemiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_b9.c
 */

#include "gba_types.h"

#define ID_NONE  0x7FFF

/* Yigin kademesi basina bir ad; ROM'da 12 baytlik iki sabit tablo. */
typedef struct NameTable {
    const char *entry[3];       /* +0x00 */
} NameTable;

/* 28 baytlik ad girisi; src/core/nodelist_b2.c'deki AreaName ile ayni,
 * +0x1A bayrak bayti eklendi. */
typedef struct AreaName {
    const char *name;           /* +0x00 */
    u8          pad04[22];      /* +0x04 */
    u8          flags;          /* +0x1A bit 3 -> gRam020004A0 = 1 */
    u8          pad1B;          /* +0x1B */
} AreaName;

/* 16 baytlik kayit grubu; src/core/nodelist_d5.c'deki AreaGroup'un
 * ikincil sayac/dizi cifti de eklenmis hali. */
typedef struct AreaGroup {
    const char *name;           /* +0x00 */
    u8          countA;         /* +0x04 */
    u8          countB;         /* +0x05 */
    u8          pad06[2];       /* +0x06 */
    u16        *idsA;           /* +0x08 */
    u16        *idsB;           /* +0x0C */
} AreaGroup;

typedef struct AreaBank {
    u8         pad00[8];        /* +0x00 */
    s32        nameCount;       /* +0x08 */
    u8         pad0C[8];        /* +0x0C */
    s32        groupCount;      /* +0x14 */
    u8         pad18[8];        /* +0x18 */
    AreaName  *names;           /* +0x20 */
    u8         pad24[8];        /* +0x24 (nodelist_d5.c'de entries) */
    AreaGroup *groups;          /* +0x2C */
} AreaBank;

/* 0x02010C60'taki 32 baytlik oturum blogu.  +0x1A bayti baska
 * ceviri birimlerinde gUnk02010C60[26] olarak okunuyor; burada
 * kullanilmadigi icin dolguda birakildi. */
typedef struct Session {
    u8          pad00[8];       /* +0x00 */
    const char *pending;        /* +0x08 bekleyen alan adi */
    u8          pad0C[4];       /* +0x0C */
    u8          depth;          /* +0x10 geri donus yigininin derinligi */
    u8          pad11[11];      /* +0x11 */
    u16         special;        /* +0x1C special tablosuna gecis bayragi */
    u8          pad1E[2];       /* +0x1E */
} Session;

/* GetRecord'un dondurdugu 60 baytlik kayit (0x08CAC248 tablosu). */
typedef struct Record {
    u8  pad00[0x20];            /* +0x00 */
    u32 f20;                    /* +0x20 */
    u8  pad24[0x18];            /* +0x24 */
} Record;

/* FUN_08053834'un kurdugu alan baglami; ic yerlesimi bu dosyada
 * kullanilmadigi icin ACILMADI (src/core/nodelist_b2.c'de AreaCtx). */
typedef struct AreaCtx AreaCtx;

extern const NameTable gEmptyAreaNames;     /* 0x083E3004 */
extern const NameTable gSpecialAreaNames;   /* 0x083E3010 */
extern const AreaName  gAreaNames[];        /* 0x08D482E0 */
extern AreaBank        gAreaBank;
extern Session         gUnk02010C60;
extern AreaCtx         gRam02035760;
extern u32             gRam020004A0;
extern u32             gRam020357E0;
extern u32             gRam02026F34;
extern u32             gRam020272C8;
extern u32             gRam02025800;

extern s32     FUN_0806dd18(const char *a, const char *b);   /* strcmp */
extern Record *GetRecord(s32 index);
extern void    BuildNodeFreeLists(void);
extern void    FUN_0805643c(void);
extern u32     FUN_08053834(AreaCtx *ctx, AreaGroup *group, const char *name);
extern void    FUN_08051154(s32 index);

/* 0x08053B44 */
void FUN_08053b44(void)
{
    NameTable   empty;
    NameTable   special;
    AreaGroup  *group;
    Record     *rec;
    const char *name;
    int         sel;            /* aramanin ham sonucu, yigin soyulmadan */
    int         slot;           /* yuklenecek grup: sel ya da derinlik-1 */
    int         rawName;        /* kural 15 ailesi: daraltma birlesmede */
    u16         nameIdx;
    int         i;
    int         j;
    int         k;              /* kural 45: her donguye ayri sayac */

    empty = gEmptyAreaNames;
    special = gSpecialAreaNames;

    /* 1. Bekleyen ad hangi kayit grubunun ikincil listesinde? */
    name = gUnk02010C60.pending;
    if (name == 0) {
        slot = ID_NONE;
        goto found;
    }
    for (i = 0; i < gAreaBank.groupCount; i++) {
        for (j = 0; j < gAreaBank.groups[i].countB; j++) {
            /* Ayri isaretci yereli: ust yorum 5. madde. */
            const AreaName *cand =
                &gAreaBank.names[gAreaBank.groups[i].idsB[j]];

            if (FUN_0806dd18(cand->name, name) == 0) {
                slot = i;
                goto found;
            }
        }
    }
    slot = ID_NONE;
found:
    sel = slot;                 /* kopya YONU onemli: ust yorum 3. madde */

    /* 2. Geri donus yigini: tepedeysek yigini bosalt, degilse tepeden
     *    bir kademe geri don ve adi yigin tablosundan al. */
    if (gUnk02010C60.depth != 0) {
        if (sel + 1 == gUnk02010C60.depth)
            gUnk02010C60.depth = 0;
        if (gUnk02010C60.depth != 0) {
            slot = gUnk02010C60.depth - 1;
            name = empty.entry[slot];
        }
    }
    if (gUnk02010C60.special != 0) {
        name = special.entry[slot];
    } else if (sel + 1 != gUnk02010C60.depth) {
        if (gUnk02010C60.depth != 0)
            name = empty.entry[slot];
    }
    gUnk02010C60.depth = 0;

    /* 3. Ayni ad, bu kez butun ad tablosunda. */
    if (name == 0) {
        rawName = ID_NONE;
        goto found2;
    }
    for (k = 0; k < gAreaBank.nameCount; k++) {
        if (FUN_0806dd18(gAreaBank.names[k].name, name) == 0) {
            rawName = k;
            goto found2;
        }
    }
    rawName = ID_NONE;
found2:
    nameIdx = rawName;          /* daraltma tam burada: 4. madde */

    if ((gAreaNames[nameIdx].flags & 8) != 0 || gUnk02010C60.special != 0)
        gRam020004A0 = 1;
    else
        gRam020004A0 = 0;

    /* 4. Kayit ve alan baglamini bastan kur. */
    group = &gAreaBank.groups[slot];
    rec = GetRecord(slot);      /* ayri yerel: ust yorum 7. madde */
    gRam020357E0 = rec->f20;
    gRam02026F34 = gRam020272C8 = 0;    /* zincirli: 6. madde */
    BuildNodeFreeLists();
    FUN_0805643c();
    FUN_08053834(&gRam02035760, group, name);
    FUN_08051154(slot);
    gRam02025800 = 0;
}
