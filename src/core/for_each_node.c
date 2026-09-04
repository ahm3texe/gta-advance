/* Liste uzerinde geri cagirma — 0x08012944-0x08012967
 *
 * Listenin her dugumu icin verilen isleyiciyi cagiriyor. Sonraki dugum
 * CAGRIDAN ONCE saklaniyor (ROM: `ldr r4,[r0,#0]` cagridan once), boylece
 * isleyici dugumu serbest birakabilir.
 *
 * `_call_via_r7` bizim yazdigimiz bir sey DEGIL: agbcc, isaretci
 * uzerinden cagriyi -mthumb-interwork ile derlerken bu koprüyu kendisi
 * uretiyor. Kaynakta duz bir isaretci cagrisi yazmak yeterli.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/for_each_node.c
 */

#include "gba_types.h"

typedef struct Node {
    struct Node *next;          /* +0x00 */
} Node;

typedef struct List {
    Node *head;                 /* +0x00 */
} List;

/* 0x08012944 */
void ForEachNode(List *list, void (*fn)(Node *, u32, u32), u32 a, u32 b)
{
    Node *node;
    Node *next;

    node = list->head;
    if (node == 0)
        return;

    do {
        next = node->next;
        fn(node, a, b);
        node = next;
    } while (node != 0);
}
