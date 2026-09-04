/* Sirali listeye ekleme — 0x0801282C-0x08012895
 *
 * Anahtara gore sirali cift bagli listeye dugum ekliyor ve sayaci
 * artiriyor. Uc yol: bos liste, araya ekleme, sona ekleme.
 *
 * Kontrol akisi ROM'daki gibi ETIKETLERLE yaziliyor. Dongu ROM'da
 * DONDURULMUS (rotated): giriste bir kez sinaniyor, govde sonunda tekrar.
 * Yapisal `while` yazmak agbcc'ye farkli blok sirasi urettiriyor -- ayni
 * durum src/world/engage_actor.c ve src/world/bump_or_reset.c'de de
 * olculmustu.
 *
 * Sayac (`count`) cagri oncesinde okunuyor ve UC yolun ucunde de ayni
 * yerden geliyor; ROM r5'te tutuyor.
 *
 * ANAHTAR PARAMETRESI u32 OLMALI, u16 DEGIL. `u16 key` yazmak agbcc'ye
 * giriste parametre kirpmasi yaptiriyor (`lsls r2,#16` + `lsrs r2,#16`,
 * tam 4 bayt) ve ROM'da o yok. `strh` zaten alt 16 biti yaziyor,
 * karsilastirma da `ldrh` sonucuyla dogal olarak unsigned kaliyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/insert_sorted.c
 */

#include "gba_types.h"

typedef struct Node {
    struct Node *next;          /* +0x00 */
    struct Node *prev;          /* +0x04 */
    u16          key;           /* +0x08 */
} Node;

typedef struct List {
    Node *head;                 /* +0x00 */
    Node *tail;                 /* +0x04 */
    u32   count;                /* +0x08 */
} List;

/* 0x0801282C */
void InsertSorted(List *list, Node *node, u32 key)
{
    Node *cur;
    Node *next;
    u32 count;

    node->key = key;

    if (list->head == 0) {
        list->head = node;
        list->tail = node;
        node->next = 0;
        node->prev = 0;
        count = list->count;
        goto done;
    }

    /* SIRA onemli: ROM once cur->next'i, SONRA sayaci okuyor. Sayaci
       once yazmak iki yuklemenin yerini degistiriyordu. */
    cur = list->head;
    next = cur->next;
    count = list->count;
    if (next == 0)
        goto check_tail;
    if (cur->key > key)
        goto insert_before;

loop:
    cur = cur->next;
    if (cur->next == 0)
        goto check_tail;
    if (cur->key <= key)
        goto loop;

check_tail:
    if (cur->key <= key)
        goto insert_after;

insert_before:
    node->next = cur;
    node->prev = cur->prev;
    if (cur->prev != 0)
        cur->prev->next = node;
    cur->prev = node;
    if (list->head == cur)
        list->head = node;
    goto done;

insert_after:
    cur->next = node;
    node->prev = cur;
    node->next = 0;
    list->tail = node;

done:
    list->count = count + 1;
}
