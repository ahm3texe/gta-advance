/* Kimlik icin dugum edinip kaydini kurma. 0x08054744, 164 bayt.
 *
 * FUN_080543D0 (nodelist_a1.c) ile ayni ailedendir: sirali listede kimligi
 * arar, bulamazsa yedek dugumu (kimligi 0x7FEF) listeden cikarip yeni
 * kimlikle geri takar. Farki, sonunda ORTAK BIR KUYRUK olmasi: dugumun
 * +0x0B bayraklarinda 2 biti varsa alan kaydina isaretci kuruluyor
 * (kimlik * 28 + gAreaBank+0x20) ve dort alan sifirlaniyor.
 *
 * DONGU BICIMI KARDESINDEN FARKLI. nodelist_a1.c'de ROM giris korumasi +
 * alttan donen do/while kullaniyor; burada DONDURULMUS `for` var
 * (`b` ile alttaki teste atlama). Ikisi de ayni ailede ama kaynakta farkli
 * yazilmislar; kalibi kardesten kopyalamak yerine ROM'dan okumak gerekiyor.
 *
 * Kural 49: "bulundu" ve "bulunamadi" govdeleri ortak kuyruga ATLIYOR,
 * akisin icine yazilmiyor.
 *
 * Kimlik * 28 carpimi `lsls #3 / subs / lsls #2` ile kuruluyor
 * (x*8 - x = x*7, sonra <<2) -- agbcc'nin sabit carpim kalibi.
 *
 * DURUM: PARK, 164/164 boyut TUTUYOR, fark 107.
 *   ilk taslak (`for` dongusu)          176/164, fark 162
 *   dongu ROM bicimine (acik atlamalar) 164/164, fark 107
 *
 * Kalan fark iki yerde: yazmac dagitimi ve NEGATIF SABITLERIN KURULUSU.
 * ROM her negatif maskeyi TAZE kuruyor (`movs r1,#2 / negs r1,r1`), bizimki
 * canli bir yazmactan turetiyor (`subs r1,#4`). Ayni fark nodelist_a1.c'de
 * de var; ortak bir kaldirac bulunursa ikisi birden ilerler.
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

    ListRemove(list, spare);
    spare->id = id;
    k = spare->kind & 15;
    spare->slot = 0;
    k = (k | 2) & ~1;
    spare->kind = k;
    InsertSorted(list, spare, id);
    node = spare;
    goto tail;

found:
    node = cur;

tail:
    k = node->kind;
    if ((k & 2) != 0) {
        node->record = gAreaBank.records + id * RECORD_SZ;
        k = k & ~1;
        node->mark = 0;
        node->init = INIT_FIELD;
        node->a = 0;
        node->b = 0;
        node->c = 0;
        node->kind = k & ~2;
    }
    return node;
}
