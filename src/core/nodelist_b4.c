/* Serbest listede kimlik arama ve zincirin ucunu dondurme
 * 0x080546CC-0x08054743  (120 bayt)
 *
 * gList02035A80 (0x02035A80) bir liste basligi: +0 bas, +4 son, +8 sayac
 * (data/ram_map.csv: gNodePoolA'nin serbest listesi). Listedeki dugumler
 * kimlige gore ARTAN sirali, tarama kimlik asilinca duruyor -- kardes
 * FindOrRecycleNode (src/core/nodelist_c1.c) ile ayni yerlesim.
 *
 * Akis:
 *   - kimlik ID_NONE ise                                  -> 0
 *   - kimlik kayit tablosunun sayacindan kucuk degilse     -> 0
 *   - listede bulunamadi                                   -> 0
 *   - bulundu -> +0x0B `ready` biti kuruluysa FUN_080521c4 cagriliyor,
 *     sonra dugumun sahibi (+0x28) varsa ve sahibin +0x0C bayraginin
 *     0. biti kuruluysa dugumun KENDISI donuyor; degilse +0x30 zinciri
 *     sonuna kadar yuruyup son halka donuyor.
 *
 * ROM'DAN OLCULEN AYRINTILAR:
 *
 * 1) Tarama dongusu gezinme isaretcisini TEK yerden yukluyor: ROM'da
 *    `ldr r0,[r0,#0]` hem ilk turda hem sonraki turlarda ayni komut
 *    (0x080546F2). Yani liste basligi Node* gibi ele alinip `cur`
 *    dogrudan basligin adresine kuruluyor ve dongu govdesine ATLANIYOR;
 *    kardes dosyadaki gibi ayri bir `list->head` okumasi YOK.
 *
 * 2) "bulundu" ve "sifir don" govdeleri kaynakta taramadan ONCE, tabloya
 *    bakan giris kontrollerinden hemen SONRA duruyor. ROM'un blok sirasi
 *    bu: giris -> `return 0` -> havuz -> bulundu -> tarama. Kural 49'un
 *    tersi bir yerlesim, bu yuzden govdeler sona TASINMADI.
 *
 * 3) Kayit tablosu ROM'da (0x08D49C00) oldugu icin sabit cast (kural 1
 *    yalniz RAM icin). Bu fonksiyon +0x04 sayaci ve +0x1C dizi tabanini
 *    kullaniyor; girdi boyu 64 bayt (`lsls r0,r2,#6`). Kardes dosya ayni
 *    tablonun +0x24 alanini 36 bayt girdiyle kullaniyor -- ayri diziler.
 *
 * 4) +0x0B bayrak bayti kardes dosyadaki bitfield yerlesimiyle ayni; ROM
 *    `movs r0,#2 / ldrb r1,[r4,#11] / ands r0,r1` sirasini uretiyor,
 *    bu tam olarak `node->ready != 0` bitfield yaziminin ciktisi.
 *
 * 5) TABLO TABANI IKI AYRI YEREL (kural 22 + kural 17). Tek `table`
 *    degiskeniyle yazinca agbcc tabani dogrudan r3'e yukluyordu:
 *      ldr r3,=0x08D49C00 / ldr r0,[r3,#4] / cmp r2,r0
 *    ROM ise tabani once r0'a alip sayaci okuyor, SONRA r3'e kopyaliyor:
 *      ldr r0,=0x08D49C00 / ldr r1,[r0,#4] / adds r3,r0,#0 / cmp r2,r1
 *    Sabiti iki AYRI yerele (`probe`, `table`) ayri deyimlerle atamak bu
 *    kopyayi uretiyor: CSE ikinci yuklemeyi kopyaya indirgiyor ama iki
 *    ayri omur kaldigi icin `probe` kisa omurlu r0'da, `table` uzun
 *    omurlu r3'te kaliyor. Tek fark buydu: 14/120 -> 0/120.
 *    `table = probe;` yazimi (kopya tabanli bolme) ise ELENIR -- iki
 *    degiskeni tek pseudoya birlestirir; sabitten YENIDEN atama sart.
 *    Sayacin da ayri yerelde (`count`) olmasi gerekiyor, yoksa karsilastirma
 *    tablo okumasindan sonraya kayiyor.
 *
 * DENEYIP ELEDIKLERIM (ROM'a karsi olculdu):
 *   - tek `table` yereli, sabit dogrudan kullanim yerinde: 14 bayt fark
 *     (yukarida 5. maddede acikli).
 *   - `while (cur != 0)` yapisal dongusu: agbcc ilk turu soyuyor, tarama
 *     govdesinde kimlik karsilastirmasi iki kez cikiyor.
 *   - `if (id >= t->count) return 0;` erken donusu: `return 0` bloku
 *     fonksiyonun sonuna dusuyor ve giris dallanmalari terse donuyor.
 *   - sahip kontrolu icin `if (owner != 0 && (owner->flags & 1) != 0)`
 *     kisa devresi: govde birlesip zincir yurumesi one geciyor.
 *
 * Kural 35'in tersi: `pop {r1}; bx r1` ve r0'da deger -> DEGER donduruyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_b4.c
 */

#include "gba_types.h"

#define ID_NONE     0x7FEF
#define ENTRY_SIZE  64

/* +0x0B bayrak bayti bit alanlarina bolunmus (kardes nodelist_c1.c ile
 * ayni). Bu fonksiyonda yalniz `ready` (bit 1) okunuyor; digerlerinin
 * anlami bilinmiyor, adlar GECICI. */
typedef struct Node {
    struct Node   *next;            /* +0x00 */
    struct Node   *prev;            /* +0x04 */
    u16            key;             /* +0x08 kimlik */
    u8             unk0A;           /* +0x0A */
    u8             low    : 1;      /* +0x0B bit 0 */
    u8             ready  : 1;      /* +0x0B bit 1 */
    u8             unk0B2 : 1;      /* +0x0B bit 2 */
    u8             unk0B3 : 1;      /* +0x0B bit 3 */
    u8             hi     : 4;      /* +0x0B bit 4-7 */
    u8             pad0C[0x1C];     /* +0x0C..0x27 anlami bilinmiyor */
    struct Owner  *owner;           /* +0x28 */
    u8             pad2C[4];        /* +0x2C */
    struct Node   *chain;           /* +0x30 zincirin sonraki halkasi */
} Node;

/* Dugumun sahibi. Yalniz +0x0C kelimesinin 0. biti okunuyor. */
typedef struct Owner {
    u8  pad00[0x0C];                /* +0x00..0x0B */
    u32 flags;                      /* +0x0C */
} Owner;

typedef struct Entry {
    u8 pad00[ENTRY_SIZE];
} Entry;

typedef struct RecordTable {
    u8     pad00[4];                /* +0x00 */
    int    count;                   /* +0x04 gecerli kimlik ust siniri */
    u8     pad08[0x14];             /* +0x08..0x1B */
    Entry *entries;                 /* +0x1C */
} RecordTable;

#define RECORD_TABLE  ((const RecordTable *)0x08D49C00)

extern Node *gList02035A80;         /* 0x02035A80 liste basligi */

extern void FUN_080521c4(Node *node, Entry *entry);

/* 0x080546CC */
Node *FUN_080546cc(int id)
{
    const RecordTable *probe;   /* kisa omurlu: yalniz sayac okumasi   */
    const RecordTable *table;   /* uzun omurlu: +0x1C dizi tabani icin */
    Node  *cur;
    Node  *node;
    Owner *owner;
    int    key;
    int    count;

    if (id == ID_NONE)
        goto none;
    probe = RECORD_TABLE;
    count = probe->count;
    table = RECORD_TABLE;
    if (id < count)
        goto scan;

none:
    return 0;

found:
    node = cur;
    goto check;

scan:
    cur = (Node *)&gList02035A80;

advance:
    cur = cur->next;
    if (cur == 0)
        goto notfound;
    key = cur->key;
    if (key == id)
        goto found;
    if (key <= id)
        goto advance;

notfound:
    node = 0;

check:
    if (node == 0)
        goto none;
    if (node->ready != 0)
        FUN_080521c4(node, table->entries + id);

    owner = node->owner;
    if (owner == 0)
        goto walk;
    if ((owner->flags & 1) != 0)
        goto done;

walk:
    while (node->chain != 0)
        node = node->chain;

done:
    return node;
}
