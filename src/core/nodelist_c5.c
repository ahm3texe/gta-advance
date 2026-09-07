/* Kimlik icin dugumu edinip alan kaydini bagla — 0x08053650, 108 bayt.
 *
 * ESLESME: 108/108 bayt (byte-matching).
 *
 * ROM okumasi:
 *   1) FUN_080543D0(&gRam02035780, id) ile sirali listeden dugum aliniyor
 *      (NULL kontrolu YOK).
 *   2) ROM bankasindaki (gAreaBank +0x20) 28 baytlik kayit dizisinden
 *      id'inci kaydin +0x1A bayrak baytinda 8 biti varsa gRam020004A0'a 1
 *      yaziliyor.
 *   3) Dugumun +0x0B baytinin ust yarisi 0x10 ise kayit isaretcisi kurulup
 *      dort alan sifirlaniyor ve +0x0B'den iki bit temizleniyor.
 *
 * KUYRUK GOVDESI src/core/nodelist_a2.c (FindOrInitAreaNode) ILE BIREBIR AYNI:
 *   record = id*28 + bank->records / mark=0 / init=0x3FF / a=b=c=0 /
 *   kind &= ~1 ... kind &= ~2.  O dosyada olculen uc mekanizma buraya
 *   dogrudan uygulandi:
 *     - negatif sabit CSE'ye dusmesin diye `m = ~1; m &= k;` ve
 *       `n = 3; n = -n; m &= n;` (negasyon AYRI deyim olmali),
 *     - isaretci aritmetigi TAMSAYI olarak yazilir
 *       `(void *)(id * 28 + (s32)bank->records)`, aksi halde gcc isaretciyi
 *       kanonik olarak basa alip `adds r0,r0,r1` yerine tersini uretiyor,
 *     - sembol adresi ayri gecicide (`bank = &gAreaBank;`) tutulur ki
 *       yukleme carpimdan ONCE yayilsin.
 *
 * `id*28` carpimi ROM'da bir kez kuruluyor (`lsls #3 / subs / lsls #2`) ve
 * r2'de iki kullanim boyunca yasiyor; buna karsilik `bank->records` IKI KEZ
 * okunuyor (`ldr rX,[r5,#32]`), cunku aradaki `gRam020004A0 = 1` yazimi
 * bellek CSE'sini gecersiz kiliyor. Ikinci okumayi kaynakta IFADE olarak
 * birakmak yeterli: yazim bellek ifadesini oldurdugu icin derleyici ilk
 * okumanin yazmacini yeniden kullanmiyor, taze `ldr` uretiyor.
 *
 * TEK OLCULEN FARK — YUKLEMENIN CARPIMA GORE YERI (8 bayt -> 0):
 *   Ilk taslak ilk kaydin adresini tek ifadede kuruyordu:
 *       rec = (AreaRecord *)(id * RECORD_SZ + (s32)bank->records);
 *   Bu 52 komutun 51'ini dogru uretiyor, ama `ldr r1,[r5,#32]` komutunu
 *   carpimdan SONRAYA atiyor; ROM onu carpimdan ONCE yayiyor. (Ayni sinif
 *   nodelist_a2.c'deki 3 numarali mekanizmada olculmustu.) Uye okumasi
 *   AYRI bir deyime alininca sira ROM'unkine oturuyor:
 *       recs = (s32)bank->records;
 *       rec  = (AreaRecord *)(id * RECORD_SZ + recs);
 *   Yani "sembol adresini gecicide tut" (`bank`) TEK BASINA yetmiyor; uye
 *   okumasinin da kendi deyimi olmasi gerekiyor. `recs` YALNIZ ilk kullanim
 *   icindir -- ikinci kullanimda da `recs` yazmak ROM'un ikinci `ldr`'sini
 *   siler.
 *
 * Kural 35: `pop {r4,r5}; pop {r0}; bx r0` -> donus tipi void.
 * Kural 1: gAreaBank / gRam020004A0 / gRam02035780 extern sembol.
 *
 * ELENEN YOL (tekrar denemeyin):
 *   - Kayit adresini tek ifadede kurmak (yukarida): 108/108 boyut TUTAR
 *     ama `ldr r1,[r5,#32]` yanlis yerde, fark 8 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_c5.c
 */

#include "gba_types.h"
#include "node_list.h"

#define RECORD_SZ   28          /* alan kaydi boyu */
#define INIT_FIELD  0x3FF       /* +0x14'e yazilan ilk deger */
#define KIND_MASK   0xF0        /* +0x0B'nin ust yarisi */
#define KIND_READY  0x10        /* kuyrugun calistigi seviye */
#define REC_FLAG    8           /* kaydin +0x1A bayragindaki kapi biti */

/* src/core/nodelist_a2.c'deki Node ile ayni yerlesim. */
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

/* 28 baytlik alan kaydi; yalniz bayrak bayti biliniyor. */
typedef struct AreaRecord {
    u8 pad00[26];
    u8 flags;                   /* 0x1A */
    u8 pad1B;
} AreaRecord;

/* Bankanin bu ceviri birimindeki gorunumu; +0x20 kayit dizisi. */
typedef struct AreaBank {
    u8  pad00[0x20];
    u8 *records;                /* 0x20 */
} AreaBank;

/* gRam02035780 node_list.h'de `NodeC4 *` olarak bildirilmis; ayni sembole
 * ikinci bir extern tur vermek check_consistency'nin ram-extern denetimine
 * takiliyor, bu yuzden adres uzerinden cast ediliyor (nodelist_a2.c ile
 * ayni cozum). */
#define NODE_LIST ((Node **)&gRam02035780)

extern AreaBank gAreaBank;
extern u32      gRam020004A0;

extern Node *FindOrClaimNode(Node **list, s32 id);

/* 0x08053650 */
void PrepareAreaNode(s32 id)
{
    AreaBank   *bank;
    AreaRecord *rec;
    Node       *node;
    s32         f;
    s32         k;
    s32         recs;
    s32         m;
    s32         n;

    node = FindOrClaimNode(NODE_LIST, id);

    bank = &gAreaBank;
    recs = (s32)bank->records;
    rec = (AreaRecord *)(id * RECORD_SZ + recs);
    f = REC_FLAG;
    f &= rec->flags;
    if (f != 0)
        gRam020004A0 = 1;

    k = node->kind;
    if ((k & KIND_MASK) == KIND_READY) {
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
}
