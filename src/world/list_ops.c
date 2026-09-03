/* Cift bagli listeye ekleme/cikarma — 0x0800DBE8-0x0800DC3F
 *
 * Dugumler baska bir struct'in icindeki alanlar: +20 next, +24 prev.
 * Bas isaretcisi 0x02016280'de.
 *
 * src/core/linked_list.c'deki desenin varyanti: burada sayaс yok ve
 * push-front (basa ekle) kullaniyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/list_ops.c
 */

#include "gba_types.h"

typedef struct Node {
    u8    pad00[20];
    struct Node *next;          /* +0x14 */
    struct Node *prev;          /* +0x18 */
} Node;

extern Node *gListHead02016280;

/* 0x0800DBE8 */
void ListPushFront(Node *node)
{
    Node *old;

    old = gListHead02016280;
    if (old == 0) {
        gListHead02016280 = node;
        node->next = 0;
        node->prev = 0;
    } else {
        node->next = old;
        node->prev = 0;
        old->prev = node;
        gListHead02016280 = node;
    }
}

/* 0x0800DC0C */
void ListRemove2(Node *node)
{
    Node *next;
    Node *prev;

    next = node->next;
    if (next != 0) {
        prev = node->prev;
        if (prev != 0) {
            next->prev = prev;
            node->prev->next = node->next;
        } else {
            next->prev = prev;
            gListHead02016280 = node->next;
        }
    } else {
        prev = node->prev;
        if (prev != 0)
            prev->next = next;
        else
            gListHead02016280 = next;
    }
}
