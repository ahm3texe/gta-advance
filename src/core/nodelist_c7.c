/* Kimlik icin serbest listeden dugum edinip zincirin sonuna yurumek
 * 0x08054608-0x080546CB  (196 bayt)
 *
 * Kardes: FUN_08054570 (src/core/nodelist_b5.c, byte-matching dogrulandi).
 * Ortak govde (kimlik tarama + yedek dugum devralma + FUN_080521c4 cagrisi)
 * oradan alindi; bu fonksiyonun ek iki parcasi var:
 *   1. Girisde SPARE_ID (0x7FEF) kimliginin kendisi reddediliyor.
 *   2. Donusten once dugumun +0x30 zinciri sonuna kadar yurunuyor.
 *
 * DENENEN / ELENEN YOLLAR  (yeni deneme yapan bunlari TEKRAR ETMESIN,
 * kendi denediklerini bu listeye EKLESIN, silmesin):
 *   - Yok; ilk yazim asagidaki bicimle birebir esletti (196/196).
 *     Asagidaki secimler ROM'dan OKUNARAK yapildi, tahminle degil:
 *     * `flag = 2; flag &= node->kind;` -- kural 33. ROM `movs r0,#2` /
 *       `ldrb r1` / `ands r0,r1` uretiyor: sonuc SABITIN yazmacinda.
 *     * `owner->flags & 1` ise DUZ yazildi. ROM burada tersini uretiyor:
 *       `ldr r0,[r0,#12]` / `movs r1,#1` / `ands r0,r1` -- sonuc DEGERIN
 *       yazmacinda. Yani kural 33 bu ikinci maskeye UYGULANMAZ; ayni
 *       fonksiyon icinde iki maske iki ayri bicim istiyor.
 *     * Kuyruk yuruyusu `node = node->link;` ile yazildi, `node = cur;`
 *       ile DEGIL. ROM `ldr r4,[r4,#48]` ile alani YENIDEN okuyor;
 *       `node = cur` yazimi bunun yerine `adds r4,r0,#0` kopyasi uretir.
 *     * Dongu bicimi kardesten KOPYALANMADI: tarama dongusu girisi
 *       atlayan `goto test`, kuyruk yuruyusu de ayri bir `goto walk_test`
 *       aliyor -- ROM'da iki ayri `b` komutu var (0x8054620, 0x80546b4).
 */

#include "gba_types.h"

#define SPARE_ID    0x7FEF
#define ENTRY_SIZE  64

/* +0x28'deki nesnenin yalnizca +0x0C bayrak kelimesi kullaniliyor;
 * geri kalani icin ad uydurulmadi, pad birakildi. */
typedef struct Owner {
    u8  pad00[0x0C];                /* +0x00 */
    u32 flags;                      /* +0x0C bit0: zincir yuruyusunu atla */
} Owner;

typedef struct Node {
    struct Node  *next;             /* +0x00 */
    struct Node  *prev;             /* +0x04 */
    u16           key;              /* +0x08 kimlik */
    u8            slot;             /* +0x0A */
    u8            kind;             /* +0x0B bayrak bayti */
    u8            pad0C[0x1C];      /* +0x0C..0x27 */
    Owner        *owner;            /* +0x28 */
    u8            pad2C[4];         /* +0x2C */
    struct Node  *link;             /* +0x30 alt/devam zinciri */
} Node;

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

extern void ListRemove(Node **list, Node *node);
extern void InsertSorted(Node **list, Node *node, s32 id);
extern void FUN_080521c4(Node *node, const Entry *entry);

/* 0x08054608 */
Node *FUN_08054608(s32 id)
{
    Node **list;
    Node  *cur;
    Node  *spare;
    Node  *node;
    Owner *owner;
    s32    k;
    s32    flag;

    /* Yedek dugumun kendi kimligi gecerli bir arama anahtari degil. */
    if (id == SPARE_ID)
        goto none;

    if (id >= RECORD_TABLE->count)
        goto none;

    list = &gList02035A80;
    cur = list[0];
    spare = list[1];
    goto test;

step:
    cur = cur->next;
test:
    if (cur == 0)
        goto scanned;
    if (cur->key == id)
        goto found;
    if (cur->key <= id)
        goto step;

scanned:
    /* Liste bitti: yedek dugum hala bostaysa devralinir. */
    if (spare->key == SPARE_ID)
        goto insert;
    goto none;

found:
    node = cur;
    goto check;

insert:
    ListRemove(list, spare);
    spare->key = id;
    k = spare->kind & 15;
    spare->slot = 0;
    k = (u8)(k | 2);
    k = k & ~1;
    spare->kind = k;
    InsertSorted(list, spare, id);
    node = spare;

check:
    if (node != 0)
        goto body;

none:
    return 0;

body:
    flag = 2;
    flag &= node->kind;
    if (flag != 0)
        FUN_080521c4(node, RECORD_TABLE->entries + id);

    /* Sahip nesnesi bit0'i kurmussa dugum oldugu gibi dondurulur. */
    owner = node->owner;
    if (owner != 0) {
        if ((owner->flags & 1) != 0)
            goto done;
    }

    /* Aksi halde +0x30 zincirinin son halkasina yurunur.
     * Kendine donen halka (cur == node) yuruyusu hic baslatmaz. */
    cur = node->link;
    if (cur == node)
        goto done;
    goto walk_test;

walk_step:
    node = node->link;
    cur = node->link;
walk_test:
    if (cur != 0)
        goto walk_step;

done:
    return node;
}
