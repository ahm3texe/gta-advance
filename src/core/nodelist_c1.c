/* Sirali dugum listesinde kimlik arama / son dugumu geri donusturme
 * 0x080547E8-0x0805486F  (136 bayt)
 *
 * gNodeListHead (0x02035A70) bir liste basligi: +0 bas, +4 son, +8 sayac
 * (src/core/linked_list.c ve src/core/insert_sorted.c ile ayni yerlesim).
 * Listedeki dugumler kimlige gore ARTAN sirali, bu yuzden tarama kimlik
 * asilinca duruyor.
 *
 * Uc sonuc yolu var:
 *   - kimlik listede bulundu           -> o dugum kullaniliyor
 *   - bulunamadi, SON dugum bos (0x7FEF) -> o dugum listeden cikarilip
 *     yeni kimlikle yeniden siraya sokuluyor
 *   - bulunamadi, son dugum dolu        -> sonuc 0
 *
 * Sonda ortak kuyruk: dugumun +0x0B bayrak baytindaki `ready` biti kuruluysa
 * FUN_08052988 cagriliyor. ROM bu kuyruga sonuc 0 iken de giriyor, yani
 * `node->ready` okumasi bos isaretci uzerinden yapilabiliyor; bu ORIJINAL
 * DAVRANIS, kaynak ona gore yazildi (yoksa blok duzeni tutmuyor).
 *
 * OLCULEN DORT AYRINTI:
 *
 * 1) Dongu ETIKETLERLE yazildi (kural 40). Yapisal `while`/`for`/`do-while`
 *    biciminin hepsi denendi: agbcc dongu icindeki referanslari dongu
 *    derinligiyle agirlikliyor, bu da anahtar degerinin oncelik puanini
 *    (2*6/4 = 3.00) gezinme isaretcisininkinin (3*12/19 = 1.90) uzerine
 *    cikariyor ve anahtar r0'i kapiyor. ROM tersini istiyor. Etiketli
 *    bicimde dongu notu olusmadigi icin agirlik kalkiyor ve dagitim
 *    ROM'unki gibi cikiyor: gezinme r0, anahtar r1.
 *
 * 2) +0x0B bayrak bayti BITFIELD olarak yazildi. Ayni ucluyu maske
 *    aritmetigiyle yazmak (`f = (f & 0x0F) | 2; f &= ~1;`) agbcc'ye
 *    maskeleri katlatiyor ya da -2'yi mevcut 2'den `sub r1,r1,#4` ile
 *    turettiriyor. Bitfield atamalari tek ldrb/strb'ye birlesiyor ve ROM'un
 *    `movs r1,#2 / negs r1,r1` ciftini uretiyor.
 *
 * 3) "Bulundu" bloku kaynakta AYRI bir etikete (`found:`) alinip
 *    "bulunamadi" blokundan SONRA yazildi. `if (key == id) { node = cur;
 *    goto check; }` biciminde agbcc kosulu ters cevirip bloku dogrudan
 *    dallanmanin ardina koyuyor (`bne` + dusme); ROM ise bloku havuzun
 *    arkasinda tutup `beq` ile atliyor. Blok sirasi kaynaktaki etiket
 *    sirasini izliyor.
 *
 * 4) Kayit tablosu ROM'da (0x08D49C00) oldugu icin sabit cast ile
 *    yaziliyor, extern sembol olarak degil (kural 1 yalniz RAM icin).
 *    Ayni tablo src/world/slot_table.c'de de bu bicimde.
 *
 * Kural 35'in tersi: `pop {r1}; bx r1` ve r0'da deger -> DEGER donduruyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_c1.c
 */

#include "gba_types.h"

#define ID_NONE       0x7FEF
#define RECORD_SIZE   36

/* +0x0B bayrak bayti bit alanlarina bolunmus. Yalniz `ready` (bit 1) ve
 * ust nibble `hi` bu fonksiyonda kullaniliyor; digerlerinin anlami henuz
 * bilinmiyor, adlar GECICI. */
typedef struct Node {
    struct Node *next;          /* +0x00 */
    struct Node *prev;          /* +0x04 */
    u16          key;           /* +0x08 kimlik */
    u8           unk0A;         /* +0x0A */
    u8           low    : 1;    /* +0x0B bit 0 */
    u8           ready  : 1;    /* +0x0B bit 1 */
    u8           unk0B2 : 1;    /* +0x0B bit 2 */
    u8           unk0B3 : 1;    /* +0x0B bit 3 */
    u8           hi     : 4;    /* +0x0B bit 4-7 */
} Node;

typedef struct List {
    Node *head;                 /* +0x00 */
    Node *tail;                 /* +0x04 */
    u32   count;                /* +0x08 */
} List;

typedef struct Record {
    u8 pad00[RECORD_SIZE];
} Record;

typedef struct RecordTable {
    u8      pad00[0x24];
    Record *records;            /* +0x24 */
} RecordTable;

#define RECORD_TABLE  ((const RecordTable *)0x08D49C00)

extern Node *gNodeListHead;     /* 0x02035A70 */

extern void ListRemove(List *list, Node *node);
extern void InsertSorted(List *list, Node *node, u32 key);
extern void FUN_08052988(Node *node, Record *record);

/* 0x080547E8 */
Node *FindOrRecycleNode(int id)
{
    List *list;
    Node *cur;
    Node *node;
    int   key;

    list = (List *)&gNodeListHead;
    cur = list->head;
    node = list->tail;
    goto test;

advance:
    cur = cur->next;

test:
    if (cur == 0)
        goto notfound;
    key = cur->key;
    if (key == id)
        goto found;
    if (key <= id)
        goto advance;

notfound:
    if (node->key == ID_NONE)
        goto reuse;
    node = 0;
    goto check;

found:
    node = cur;
    goto check;

reuse:
    ListRemove(list, node);
    node->key = id;
    node->hi = 0;
    node->unk0A = 0;
    node->ready = 1;
    node->low = 0;
    InsertSorted(list, node, id);

check:
    if (node->ready != 0)
        FUN_08052988(node, RECORD_TABLE->records + id);

    return node;
}
