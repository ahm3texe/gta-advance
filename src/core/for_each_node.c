/* Callback over the list — 0x08012944-0x08012967
 *
 * It calls the given handler for every node of the list. The next node is saved
 * BEFORE THE CALL (ROM: `ldr r4,[r0,#0]` before the call), so the handler is
 * free to release the node.
 *
 * `_call_via_r7` is NOT something we wrote: agbcc produces this bridge itself
 * when compiling a call through a pointer with -mthumb-interwork. Writing a
 * plain pointer call in the source is enough.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/for_each_node.c
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
