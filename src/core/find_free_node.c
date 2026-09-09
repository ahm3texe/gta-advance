/* Search the free node list for an id — 0x08055954-0x0805596F
 *
 * INSTRUCTION FOR INSTRUCTION the same as FindNode (src/world/node_search.c,
 * 0x08055AA8); the only difference is the list head: gNodeListHead
 * (0x02035A70) there, gList02035A80 here. tools/find_twins.py pointed at it
 * with 92.9% similarity.
 *
 * The list is ORDERED by id; the search returns 0 once the id is passed.
 * Because the header object's +0x00 is the list's first node, the loop starts
 * directly from the header address -- a single `ldr r0,[r0,#0]` does both the
 * start and the advance.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/find_free_node.c
 */

#include "gba_types.h"

typedef struct Node {
    struct Node *next;          /* +0x00 */
    u8           pad04[4];
    u16          id;            /* +0x08 */
} Node;

extern Node *gList02035A80;

/* 0x08055954 */
Node *FindFreeNode(int id)
{
    Node *node;
    int found;

    node = (Node *)&gList02035A80;
    for (;;) {
        node = node->next;
        if (node == 0)
            return 0;

        found = node->id;
        if (found == id)
            return node;
        if (found > id)
            return 0;
    }
}
