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
 * DURUM: PARK, 164/164 boyut TUTUYOR, fark 6/164 (76/79 komut ayni).
 *   ilk taslak (`for` dongusu)           176/164, fark 162
 *   dongu ROM bicimine (acik atlamalar)  164/164, fark 107 -> 80
 *   kuyrukta ayri maske yereli (m)       fark 80 -> 78, kuyruk yazmaclari
 *                                        (node r3, k r4) TAM oturdu
 *   kuyruk son maskesi `n = 3; n = -n;`  fark 78 (tek basina), asagidaki
 *                                        insert duzeltmesiyle birlikte 11
 *   insert maskesi `q = ~1;` ayri deyim  fark 11
 *   kayit adresi tamsayi + `bank` gecici fark 6
 *
 * ---------------------------------------------------------------------
 * BU OTURUMDA OLCULEN UC MEKANIZMA (hepsi tekrar kullanilabilir)
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
 * (2) NEGATIF SABIT: CSE YI ONLEMEK ICIN SABITI ONCE KUR (fark 78 -> 11)
 *     agbcc, canli bir kucuk sabitten turetilebilen negatif sabiti tek
 *     komuta indiriyor: r1=2 canliyken -2 icin `subs r1,#4`; r1=0
 *     canliyken -3 icin `subs r1,#3 / adds r0,r1,#0`. ROM ise ikisini de
 *     TAZE kuruyor (`movs #2 / negs`, `movs #3 / negs`).
 *     OLCULEN KURAL: karar, and/or genislemesindeki force_reg SIRASINA
 *     bagli. Negatif sabit RTL de otekinden ONCE dogarsa CSE tablosunda
 *     esi yoktur ve iki komutlu bicim korunur. Isleyen yazimlar:
 *         q = ~1;             k = (k | 2) & q;     (-2, 2 den ONCE dogar)
 *         n = 3;  n = -n;     m &= n;              (-3 taze)
 *     `n = ~2;` TEK deyim olarak YETMEZ; negasyon AYRI deyim olmali.
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
 * KALAN FARK (6 bayt, 3 komut) -- insert blogundaki -2 nin YERI
 * ---------------------------------------------------------------------
 *     ROM  : movs r1,#2 / orrs r0,r1 / movs r1,#2 / negs r1,r1 / ands r0,r1
 *     bizim: movs r2,#2 / negs r2,r2 / movs r1,#2 / orrs r0,r1 / ands r0,r2
 *     Sabit dogru BICIMDE (movs+negs) kuruluyor ama ORRS ten ONCE ve r1
 *     yerine r2 de. r2 ye dusme sebebi: -2 nin omru OR un 2 sini kapsiyor,
 *     cakisiyorlar; 2 daha yuksek oncelikli oldugu icin r1 i aliyor.
 *     ROM da -2 OR dan SONRA doguyor, omurler ayrik, ikisi de r1 e siginiyor.
 *     KILIT CELISKI: (2) numarali mekanizma -2 yi 2 den ONCE dogurmayi
 *     GEREKTIRIYOR, ROM ise bu blokta -2 yi SONRA doguruyor. -2 yi OR dan
 *     sonraya alan her yazim `subs r1,#4` e dusuyor. Bu iki sarti ayni
 *     anda saglayan bir kaynak-duzeyi kaldirac BULUNAMADI.
 *
 * ---------------------------------------------------------------------
 * DENENIP ELENEN YOLLAR (tekrar denemeyin)
 * ---------------------------------------------------------------------
 *  - `m = k & ~1;` / `m = ~1 & k;` / `m = -2 & k;` / `m = k & 0xFFFFFFFE;`
 *    / `m = (k | 0) & ~1;` -- dordu de tek pseudo ya birlesiyor, fark 80.
 *  - `n = ~2;` tek deyim (gecici degiskenli ya da degiskensiz) -- CSE yine
 *    `subs r1,#3` uretiyor, fark 78.
 *  - `n = 2; n = -n;` -- ANLAMCA YANLIS (~1 verir, ~2 degil). Bayt farki 46
 *    gorunuyor ama tesadufi; kullanmayin.
 *  - insert maskesini `k = k | 2;` + `k = k & q;` diye BOLMEK -- iki tane 2
 *    birlesiyor, cikti 160 bayt (bir komut eksik), fark 92.
 *  - `k = (k | 2) & (q = ~1);` -- atama ifadenin icinde de OR dan sonra
 *    genisliyor, CSE fire ediyor, fark 78.
 *  - `k = (k & ~1) | 2;` (OR/AND takasi; bitler bagimsiz, anlam ayni)
 *    -- fark 7, mevcut 6 dan kotu.
 *  - `q = 2; k = (k | q) & ~q;` -- 156 bayt, fark 90.
 *  - Isaretci aritmetiginin BUTUN oteki yazimlari: operand takasi,
 *    `(id*7)*4`, `(id*7 << 2)`, `((id<<3)-id)*4`, `&records[id*28]`,
 *    28 baytlik AreaRec dizisi + `&records[id]`, `off` ve `base` gecicileri
 *    -- (3) teki tamsayi+bank ciftinden BASKA hicbiri yazmaclari
 *    degistirmiyor; hepsi 11 de sabit kaliyor.
 *  - insert blogundaki 6 deyimin GECERLI TUM SIRALAMALARI (perm taramasi,
 *    iki kez: 11 ve 6 tabaninda) -- en iyisi mevcut sira; digerleri 9+.
 *  - 8 yerel bildiriminin 576 SIRALAMASI (4 isaretci! x 4 s32!) -- hicbiri
 *    6 nin altina inmiyor. Bildirim sirasi bu fonksiyonda kaldirac DEGIL.
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
    u8    kind;                 /* 0x0B */
    u8    pad0C[8];
    u16   init;                 /* 0x14 */
    u8    pad16;
    u8    mark;                 /* 0x17 */
    void *record;               /* 0x18 */
    s32   a;                    /* 0x1C */
    s32   b;                    /* 0x20 */
    s32   c;                    /* 0x24 */
} Node;

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
Node *FUN_08054744(s32 id)
{
    Node **list;
    Node *cur;
    Node *spare;
    Node *node;
    s32 k;
    s32 m;
    s32 n;
    s32 q;
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
    k = spare->kind & 15;
    spare->slot = 0;
    q = ~1;
    k = (k | 2) & q;
    spare->kind = k;
    InsertSorted(list, spare, id);
    node = spare;
    goto tail;

tail:
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
