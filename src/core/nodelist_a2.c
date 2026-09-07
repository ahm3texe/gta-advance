/* Kimlik icin dugum edinip kaydini kurma. 0x08054744, 164 bayt.
 *
 * FUN_080543D0 (nodelist_a1.c) ile ayni ailedendir: sirali listede kimligi
 * arar, bulamazsa yedek dugumu (kimligi 0x7FEF) listeden cikarip yeni
 * kimlikle geri takar. Farki, sonunda ORTAK BIR KUYRUK olmasi: dugumun
 * +0x0B bayraklarinda 2 biti varsa alan kaydina isaretci kuruluyor
 * (kimlik * 28 + gAreaBank+0x20) ve dort alan sifirlaniyor.
 *
 * DONGU BICIMI KARDESINDEN FARKLI. nodelist_a1.c de ROM giris korumasi +
 * alttan donen do/while kullaniyor; burada DONDURULMUS `for` var
 * (`b` ile alttaki teste atlama). Ikisi de ayni ailede ama kaynakta farkli
 * yazilmislar; kalibi kardesten kopyalamak yerine ROM dan okumak gerekiyor.
 *
 * Kural 49: "bulundu" ve "bulunamadi" govdeleri ortak kuyruga ATLIYOR,
 * akisin icine yazilmiyor.
 *
 * Kimlik * 28 carpimi `lsls #3 / subs / lsls #2` ile kuruluyor
 * (x*8 - x = x*7, sonra <<2) -- agbcc nin sabit carpim kalibi.
 *
 * DURUM: ESLESTI. 164/164 bayt, 79/79 komut.
 *   ilk taslak (`for` dongusu)           176/164, fark 162
 *   dongu ROM bicimine (acik atlamalar)  164/164, fark 107 -> 80
 *   kuyrukta ayri maske yereli (m)       fark 80 -> 78, kuyruk yazmaclari
 *                                        (node r3, k r4) TAM oturdu
 *   kuyruk son maskesi `n = 3; n = -n;`  fark 78 (tek basina), asagidaki
 *                                        insert duzeltmesiyle birlikte 11
 *   insert maskesi `q = ~1;` ayri deyim  fark 11
 *   kayit adresi tamsayi + `bank` gecici fark 6
 *   insert bayrak baytini BITFIELD yazma fark 6 -> 0   <-- son adim
 *
 * ---------------------------------------------------------------------
 * SON ADIM: +0x0B BAYRAK BAYTI MASKE ARITMETIGIYLE YAZILAMIYOR
 * ---------------------------------------------------------------------
 * Kalan 6 bayt (3 komut) insert blogundaki -2 nin YERI idi:
 *     ROM  : movs r1,#2 / orrs r0,r1 / movs r1,#2 / negs r1,r1 / ands r0,r1
 *     bizim: movs r2,#2 / negs r2,r2 / movs r1,#2 / orrs r0,r1 / ands r0,r2
 * Yani ROM -2 yi OR dan SONRA ve 2 ile AYNI yazmacta (r1) taze kuruyor.
 *
 * NEDEN MASKE ARITMETIGI BUNU URETEMEZ (derleyici kaynagindan dogrulandi):
 * reload sonrasi `reload_cse_move2add` (gcc/reload1.c) her donanim yazmacinin
 * bilinen sabit degerini izliyor ve `(set R sabit)` komutunu daha ucuzsa
 * `(set R (plus R fark))` e ceviriyor. Thumb CONST_COSTS ile
 * cost(-2, SET) = 3 komut, cost(-4, PLUS) = 0 -> DAIMA katlaniyor:
 * r1 zaten 2 tutuyorken -2 yazmak `subs r1,#4` e dusuyor. Ayni izleyici
 * gereksiz ikinci `movs r1,#2` yi de (fark 0) no-op a cevirip sildiriyor.
 * Yani "-2 yi OR dan sonra kur" ile "movs+negs bicimini koru" sartlari
 * KAYNAK DUZEYINDE ayni anda saglanamiyor -- izleyiciyi ancak arada bir
 * CODE_LABEL gecersiz kilar, C den oyle bir etiket uretilemiyor
 * (denendi: `goto mask; mask:` ve ciplak etiket, ilk jump gecisi siliyor).
 *
 * COZUM: bayrak baytini UC AYRI BITFIELD ATAMASI olarak yazmak. agbcc bunu
 * TEK ldrb/strb ciftine birlestiriyor ve maskeleri sirayla, her birini TAZE
 * sabitle kuruyor; move2add in izledigi "ayni yazmacta bilinen sabit"
 * durumu olusmadigi icin -2 `movs #2 / negs` olarak kaliyor.
 * ESLESEN KARDES KANITI: src/core/nodelist_c1.c (FindOrRecycleNode) ve
 * src/core/nodelist_b5.c (GetOrCreateRecordNode) ROM da 0x08054830 ve 0x080545C4 te
 * BAYT BAYT AYNI diziyi tasiyor ve ikisi de bitfield yazimiyla eslesmis.
 * nodelist_c1.c basligindaki 2. madde bu tuzagi zaten kaydetmis.
 *
 * IKI GORUNUM GEREKIYOR: kuyruk blogu ayni bayti TEK ldrb ile okuyup hem
 * `& 2` testinde hem iki maskede kullaniyor, sonra TEK strb ile yaziyor;
 * orayi bitfield e cevirmek ikinci bir ldrb dogurup eslesmeyi bozuyor.
 * Bu yuzden `Node.kind` bayt olarak duruyor (kuyruk gorunumu) ve insert
 * blogu ayni nesneye `NodeBits` uzerinden bakiyor (bit gorunumu).
 *
 * ---------------------------------------------------------------------
 * ONCEKI OTURUMDA OLCULEN UC MEKANIZMA (hepsi tekrar kullanilabilir)
 * ---------------------------------------------------------------------
 *
 * (1) AYRI MASKE YERELI YAZMAC DAGITIMINI KAYDIRIR  (fark 80 -> 78)
 *     ROM kuyrukta node u r3, k yi r4 tutuyordu; bizimki r2 ve r3
 *     veriyordu. Sebep: ROM da `kind & ~1` sonucu AYRI bir blok-yerel
 *     pseudo (r2), bizimkinde k nin uzerine yaziliyordu; yani blok 10 da
 *     ayni anda yalnizca IKI yerel canliydi. Ucuncu yerel dogunca yerel
 *     dagitici (local-alloc, global dagiticidan ONCE kosar) r2 yi kapatiyor,
 *     global dagitici de node a r3, k ya r4 veriyor.
 *     ONEMLI: `m = k & ~1;` YETMEZ -- k oldugu icin regmove hedefi k yapip
 *     pseudo yu birlestiriyor. Ayri pseudo icin sabiti ONCE kendi deyiminde
 *     kurmak sart:   m = ~1;   m &= k;
 *     Bu, "kopya tabanli bolme her zaman elenir" kuralinin ISLEYEN karsiti:
 *     bolme SABITTEN baslarsa (degerden degil) yeni allocno doguyor.
 *
 * (2) NEGATIF SABIT: SABITI ONCE KUR (fark 78 -> 11)
 *     agbcc, canli bir kucuk sabitten turetilebilen negatif sabiti tek
 *     komuta indiriyor: r1=2 canliyken -2 icin `subs r1,#4`; r1=0
 *     canliyken -3 icin `subs r1,#3 / adds r0,r1,#0`. ROM ise ikisini de
 *     TAZE kuruyor (`movs #2 / negs`, `movs #3 / negs`).
 *     Bunu yapan gecis yukarida acildigi gibi `reload_cse_move2add`;
 *     ilac, negatif sabiti ONCE kurup canli sabitle omrunu ayirmak:
 *         q = ~1;             k = (k | 2) & q;     (-2, 2 den ONCE dogar)
 *         n = 3;  n = -n;     m &= n;              (-3 taze)
 *     `n = ~2;` TEK deyim olarak YETMEZ; negasyon AYRI deyim olmali.
 *     (Insert blogunda bu ilac kaldirildi -- oraya bitfield geldi; kuyrukta
 *     hala gecerli ve gerekli.)
 *
 * (3) ISARETCI ARITMETIGINDE OPERAND SIRASI + YUKLEME SIRASI (11 -> 6)
 *     ROM: `ldr r1,[pc]` (=&gAreaBank) ONCE, sonra carpim r0 da, sonra
 *     `ldr r1,[r1,#32]`, sonra `adds r0,r0,r1`. Yani toplamanin ILK
 *     operandi CARPIM ve sonuc onun yazmacini paylasiyor.
 *     `records + id*28` yazildiginda GCC isaretciyi kanonik olarak basa
 *     aliyor; operandlari elle takas etmek (`id*28 + records`) HICBIR SEY
 *     DEGISTIRMIYOR. Kanoniklestirmeyi kirmak icin iki tarafi da TAMSAYI
 *     yapmak gerekiyor:  (void *)(id * RECORD_SZ + (s32)bank->records)
 *     -- bu yazmaclari duzeltiyor ama sembol yuklemesini carpimdan SONRAYA
 *     atiyor. Yukleme sirasini geri almak icin sembol adresi ayri bir
 *     gecicide tutuluyor:  bank = &gAreaBank;
 *     Ikisi BIRLIKTE ROM un hem sirasini hem yazmaclarini veriyor.
 *
 * ---------------------------------------------------------------------
 * DENENIP ELENEN YOLLAR (tekrar denemeyin)
 * ---------------------------------------------------------------------
 *  - `m = k & ~1;` / `m = ~1 & k;` / `m = -2 & k;` / `m = k & 0xFFFFFFFE;`
 *    / `m = (k | 0) & ~1;` -- dordu de tek pseudo ya birlesiyor, fark 80.
 *  - `n = ~2;` tek deyim (gecici degiskenli ya da degiskensiz) -- yine
 *    `subs r1,#3` uretiyor, fark 78.
 *  - `n = 2; n = -n;` -- ANLAMCA YANLIS (~1 verir, ~2 degil). Bayt farki 46
 *    gorunuyor ama tesadufi; kullanmayin.
 *  - Isaretci aritmetiginin BUTUN oteki yazimlari: operand takasi,
 *    `(id*7)*4`, `(id*7 << 2)`, `((id<<3)-id)*4`, `&records[id*28]`,
 *    28 baytlik AreaRec dizisi + `&records[id]`, `off` ve `base` gecicileri
 *    -- (3) teki tamsayi+bank ciftinden BASKA hicbiri yazmaclari
 *    degistirmiyor; hepsi 11 de sabit kaliyor.
 *  - insert blogundaki 6 deyimin GECERLI TUM SIRALAMALARI (perm taramasi,
 *    iki kez: 11 ve 6 tabaninda) -- en iyisi eski sira; digerleri 9+.
 *  - 8 yerel bildiriminin 576 SIRALAMASI (4 isaretci! x 4 s32!) -- hicbiri
 *    6 nin altina inmiyor. Bildirim sirasi bu fonksiyonda kaldirac DEGIL.
 *
 *  BU OTURUMDA (insert maskesini maske aritmetigiyle kurtarma denemeleri,
 *  HEPSI 6 dan kotu; hicbiri tekrar denenmemeli):
 *  - `k = (k | 2) & ~1;` tek deyim: 78. `k = (k & ~1) | 2;`: 7.
 *  - OR ile AND i AYRI deyimlere bolmenin her bicimi (`q` once, `q` arada,
 *    `q = -2`, `q = 1; q = ~q`, `k |= 2; k &= q;`): 160 bayt / fark 96 --
 *    iki `2` birlesip `sub r0,r0,#4` cikiyor.
 *  - 2 ve -2 icin AYRI yereller + yeni yerelin 10 bildirim konumu (F1/F2
 *    x 10 konum, 20 olcum): hepsi 160/96.
 *  - 2 ve -2 icin {q,m,n,t,u} ikili kombinasyonlarinin tamami (20 olcum):
 *    yalniz `m` (kuyrukta da tanimli) katlanmayi onluyor ama m in allocno su
 *    iki bloga yayilip callee-saved e (r5) tasiniyor, r7 de push a giriyor:
 *    164 bayt / fark 42. ROM da iki maske AYRI yazmaclarda (r1 ve r2),
 *    dolayisiyla degisken paylasimi YAPISAL OLARAK yanlis.
 *  - Ayni blokta `q` ye ikinci/ucuncu atama (`q = 0; spare->slot = q;`
 *    + `q = 2;` + `q = ~1;`): 160/96. Cok atamali pseudo katlanmayi
 *    ONLEMIYOR; belirleyici olan ayni DONANIM yazmacinin yeniden
 *    kullanilmasi.
 *  - `q = 2; k = k | q; t = 2; t = -t;` (negasyonu ayri deyim): iki `2`
 *    CSE ile birlesiyor, `neg` kaliyor ama komut sayisi 4 -> 160/96.
 *  - `q`/`t`/`k` yi u8/s8/u16/s16 yapmak (16 olcum): en iyi 78.
 *  - `kind` alanini s8 yapmak, maskeyi dogrudan alanda bilesik atamayla
 *    yazmak (`spare->kind &= ~1;`), `k` yi u8/s8 yapmak (30 olcum):
 *    en iyi 21 (`movs #254` tek komuta katlaniyor, ROM iki komut istiyor).
 *  - Etiketle blok siniri acmak (`goto mask; mask:`, ciplak `mask:`,
 *    ikinci `goto`): ilk jump gecisi etiketi siliyor, 160/96.
 *  - Bitfield leri Node un icine koyup ayrica `all` uyesi olan bir UNION
 *    kurmak: union hizalamasi bayti 0x0B den 0x0C ye itiyor, fark 16.
 *    Ayri `NodeBits` gorunumu sart.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_a2.c
 */

#include "gba_types.h"
#include "node_list.h"

#define SPARE_ID   0x7FEF
#define RECORD_SZ  28
#define INIT_FIELD 0x3FF

typedef struct Node {
    struct Node *next;          /* 0x00 */
    u8    pad04[4];
    u16   id;                   /* 0x08 */
    u8    slot;                 /* 0x0A */
    u8    kind;                 /* 0x0B bayrak bayti, BAYT gorunumu */
    u8    pad0C[8];
    u16   init;                 /* 0x14 */
    u8    pad16;
    u8    mark;                 /* 0x17 */
    void *record;               /* 0x18 */
    s32   a;                    /* 0x1C */
    s32   b;                    /* 0x20 */
    s32   c;                    /* 0x24 */
} Node;

/* +0x0B in BIT gorunumu. Ayni nesne, ikinci bir tur; `Node` icine union
 * olarak konamiyor cunku union hizalamasi alani 0x0C ye itiyor.
 * Bit adlari kardes src/core/nodelist_c1.c ile ayni; `unk2`/`unk3` un
 * anlami bilinmiyor, adlar GECICI. Insert blogu bu gorunumu kullaniyor:
 * ucu bir arada yazilinca agbcc tek ldrb/strb ciftine katliyor ve ROM un
 * `movs #15 / ands` + `movs #2 / orrs` + `movs #2 / negs / ands` dizisini
 * aynen uretiyor (bkz. baslikteki "SON ADIM"). */
typedef struct NodeBits {
    u8 pad00[11];               /* 0x00..0x0A */
    u8 low   : 1;               /* 0x0B bit 0 */
    u8 ready : 1;               /* 0x0B bit 1 */
    u8 unk2  : 1;               /* 0x0B bit 2 */
    u8 unk3  : 1;               /* 0x0B bit 3 */
    u8 hi    : 4;               /* 0x0B bit 4-7 */
} NodeBits;

/* gRam02035780 include/node_list.h'de `NodeC4 *` olarak bildirilmis
 * (liste BASI, +0x00).  Yedek dugum +0x04'te; ayni sembole ikinci bir
 * extern tur vermek check_consistency'nin `ram-extern` denetimine takiliyor,
 * bu yuzden paylasilan bildirim uzerinden ikinci kelimeye eriliyor. */
#define NODE_LIST ((Node **)&gRam02035780)

typedef struct AreaBank {
    u8    pad00[0x20];
    u8   *records;              /* 0x20 */
} AreaBank;

extern AreaBank gAreaBank;
extern void ListRemove(Node **list, Node *node);
extern void InsertSorted(Node **list, Node *node, s32 id);

/* 0x08054744 */
Node *FindOrInitAreaNode(s32 id)
{
    Node **list;
    Node *cur;
    Node *spare;
    Node *node;
    NodeBits *bits;
    s32 k;
    s32 m;
    s32 n;
    AreaBank *bank;

    list = NODE_LIST;           /* ROM tabani r6'da TUTUYOR */
    cur = list[0];
    spare = list[1];
    goto test;                  /* ROM `b` ile alttaki teste atliyor */

step:
    cur = cur->next;
test:
    if (cur == 0) goto scanned;
    if (cur->id == id) goto found;
    if (cur->id <= id) goto step;

scanned:
    if (spare->id != SPARE_ID) {
        node = 0;
        goto tail;
    }
    goto insert;

    /* Kural 49: ROM'da `found` blogu literal havuzunun HEMEN ardinda,
     * insert govdesinden ONCE duruyor. */
found:
    node = cur;
    goto tail;

insert:
    ListRemove(list, spare);
    spare->id = id;
    /* Bayrak bayti: ust yariyi sil, "hazir" bitini kur, 0. biti sil.
     * Sira ROM'un komut sirasidir; maske aritmetigiyle yazilamaz. */
    bits = (NodeBits *)spare;
    bits->hi = 0;
    spare->slot = 0;
    bits->ready = 1;
    bits->low = 0;
    InsertSorted(list, spare, id);
    node = spare;
    goto tail;

tail:
    /* Kuyruk ayni bayti TEK ldrb ile okuyup hem testte hem iki maskede
     * kullaniyor, sonra TEK strb ile yaziyor -- bu yuzden burada BAYT
     * gorunumu ve maske aritmetigi sart, bitfield DEGIL. */
    k = node->kind;
    if ((k & 2) != 0) {
        bank = &gAreaBank;
        node->record = (void *)(id * RECORD_SZ + (s32)bank->records);
        m = ~1;
        m &= k;
        node->mark = 0;
        node->init = INIT_FIELD;
        node->a = 0;
        node->b = 0;
        node->c = 0;
        n = 3;
        n = -n;
        m &= n;
        node->kind = m;
    }
    return node;
}
