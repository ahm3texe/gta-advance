/* gListHead02016280 listesini anahtara gore siralama — 0x0800D530-0x0800D65F
 *
 * Ne yapiyor: 0x02016280'deki cift bagli listeyi bosaltip 0x02016288'deki
 * ikinci listeye ekleme siralamasiyla (insertion sort) yeniden diziyor,
 * sonunda sirali listenin basini 0x02016280'e geri yaziyor.
 *
 * Siralama anahtari 32 bit: ust yariya FUN_0800d450'nin dondurdugu puan
 * (dugumun +0x1C alanina da yaziliyor), alt yariya 255 - dugumun sahip
 * kaydindaki (+0x10 -> +0x14) sira degeri konuyor. Karsilastirmalar
 * ISARETSIZ (`bls`/`bhi`), yani anahtar u32. Liste anahtara gore AZALAN
 * sirali.
 *
 * Ekleme noktasi bastan taranmiyor: bir onceki eklenen dugumden (prev)
 * baslayip anahtar buyukse ileri (next), kucuk/esitse geri (prev) yonde
 * yuruyor. Iki tarama simetrik yazildi, ROM'daki iki ayri dongu bu.
 *
 * Dugum yerlesimi: +0x10 sahip kaydi, +0x14 next, +0x18 prev, +0x1C puan.
 * +0x14/+0x18 src/world/list_ops.c'deki Node ile ayni; sahip kaydinin
 * +0x14 alani FUN_0800d450'nin de okudugu blok.
 *
 * ADRES NOTU: 0x02016280 data/ram_map.csv'de kayitli, extern sembol
 * olarak kullanildi (kural 1). 0x02016288 KAYITLI DEGIL ve bu oturumda
 * data/ altina yazmak yasak, bu yuzden src/core/list_b1.c'deki gibi
 * #define ile adres cast'i yazildi. Katlanma tuzagi burada olusmuyor:
 * adresten hicbir ofset kullanilmiyor, ROM da onu her seferinde AYRI
 * literal olarak yukluyor. Havuz yerlesimi (0x02016280, 0x02016288,
 * 0x02016288, 0x02016280, 0x02016288) ROM'unkiyle birebir cikti.
 *
 * OLCULEN TEK AYRINTI -- `sorted` yereli neden var:
 * Ilk surum 296/304 uretti ve TEK fark register numaralariydi: bizde
 * sabit 255 r7'de, `next` r8'de; ROM'da 255 r8'de, `next` r9'da. Sekiz
 * baytin tamami bu kaymanin bedeli (prolog +2, `movs r0,#255`/`mov r8,r0`
 * +2, `mov r2,r8` +2, epilog +2) -- komut dizisi zaten aynidiydi.
 * Sebep: ROM 0x02016288 ADRESINI ileri taramanin basinda bir pseudo'ya
 * alip dongu boyunca canli tutuyor (r7), biz ise karsilastirmanin hemen
 * onunde kisa omurlu bir scratch'a (r1) aliyorduk. Fazladan bir canli
 * deger, callee-saved listesini bir kaydiriyor (docs/COMPILER.md register
 * tablosu). Adresi `sorted = gSortedHeadPtr;` ile ACIK bir yerele almak
 * -- ve karsilastirma ile geri yazimi `*sorted` uzerinden yapmak -- o
 * omru uretti: 296 -> 304, birebir.
 * Yerin onemli: atama `q = prev->next;`den SONRA, dongudEN once olmali;
 * ROM'da `ldr r7,=...` tam o iki komutun arasinda duruyor.
 * Geri taramadaki bas-a-ekleme ve fonksiyon sonundaki geri yazim
 * `sorted`i KULLANMIYOR (makroyu kullaniyor); ROM oralarda adresi yeniden
 * havuzdan yukluyor, `sorted`i oraya da tasimak omru uzatir ve bozar.
 *
 * ELENEN YOL: 0x02016288'i extern sembol yapmak (kural 1'in dogal
 * secimi) denenemedi -- data/ram_map.csv'ye yazmak bu oturumda yasak.
 * Gerek de kalmadi: #define adres cast'i, yukaridaki `sorted` yereliyle
 * birlikte ROM'un uretmis oldugu kodun aynisini veriyor.
 *
 * ESLESME: 304/304 bayt, ikinci denemede.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/listhead_e1.c
 */

#include "gba_types.h"

/* Alt yarinin tumleyeni: kucuk sira degeri buyuk anahtar demek. */
#define RANK_BIAS 255

/* Dugumun +0x10'da gosterdigi kayit; yalniz +0x14 kullaniliyor. */
typedef struct Owner {
    u8  pad00[20];
    u32 rank;                   /* +0x14 */
} Owner;

typedef struct Node {
    u8  pad00[16];
    Owner *owner;               /* +0x10 */
    struct Node *next;          /* +0x14 */
    struct Node *prev;          /* +0x18 */
    u16 score;                  /* +0x1C */
} Node;

/* Sirali listenin basi; data/ram_map.csv'de kaydi yok (bkz. ADRES NOTU). */
#define gSortedHeadPtr       ((Node **)0x02016288)
#define gSortedHead02016288  (*gSortedHeadPtr)

extern Node *gListHead02016280;

/* 0x0800D450: dugumun kamera/bolge kaydina uzakligindan puan uretiyor. */
extern int FUN_0800d450(Node *node);

#define SORT_KEY(n) \
    (((u32)(n)->score << 16) | (u32)(RANK_BIAS - (n)->owner->rank))

/* 0x0800D530 */
void SortListByKey(void)
{
    Node *cur;
    Node *next;
    Node *prev;
    Node *p;
    Node *q;
    Node *head;
    Node **sorted;
    u32 key;
    int score;

    if (gListHead02016280 == 0)
        return;

    /* Ilk dugum sirali listenin tek elemani olarak kuruluyor. */
    cur = gListHead02016280->next;
    gSortedHead02016288 = gListHead02016280;
    gListHead02016280->next = 0;
    gListHead02016280->prev = 0;
    gSortedHead02016288->score = FUN_0800d450(gListHead02016280);
    prev = gListHead02016280;

    while (cur != 0) {
        next = cur->next;
        score = FUN_0800d450(cur);
        cur->score = score;
        key = ((u32)score << 16) | (u32)(RANK_BIAS - cur->owner->rank);

        p = prev;
        if (SORT_KEY(prev) > key) {
            /* Ileri tarama: anahtari buyuk olan son dugumun ardina. */
            q = prev->next;
            sorted = gSortedHeadPtr;
            while (q != 0 && SORT_KEY(q) > key) {
                p = q;
                q = q->next;
            }
            if (q == *sorted) {
                cur->next = q;
                cur->prev = 0;
                q->prev = cur;
                *sorted = cur;
            } else {
                cur->next = q;
                cur->prev = p;
                p->next = cur;
                if (q != 0)
                    q->prev = cur;
            }
        } else {
            /* Geri tarama: anahtari kucuk/esit olanlarin onune. */
            q = prev->prev;
            while (q != 0 && SORT_KEY(q) <= key) {
                p = q;
                q = q->prev;
            }
            if (q == 0) {
                head = gSortedHead02016288;
                cur->next = head;
                cur->prev = 0;
                head->prev = cur;
                gSortedHead02016288 = cur;
            } else {
                cur->next = p;
                cur->prev = q;
                p->prev = cur;
                q->next = cur;
            }
        }

        prev = cur;
        cur = next;
    }

    gListHead02016280 = gSortedHead02016288;
}
