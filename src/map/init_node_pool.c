/* Thread 128 nodes onto the list — 0x08042790-0x080427B7
 *
 * The array at 0x0202F4D0 holds 128 nodes of 28 bytes each, and this walks it
 * BACKWARDS by count while walking the pointer forwards, so the list ends up in
 * reverse order. The two together fix the array's extent in data/ram_map.csv:
 * 0x0202F4D0 + 128 * 28 lands exactly on the list header at 0x020302D0.
 *
 * The countdown is signed (`cmp r4,#0 / bge`), so it runs 127 down to 0 and one
 * more turn at 0, which is 128 nodes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/map/init_node_pool.c
 */

#include "gba_types.h"

#define NODE_COUNT   128
#define NODE_STRIDE  28

typedef struct Node2 {
    struct Node2 *next;         /* +0x00 */
    struct Node2 *prev;         /* +0x04 */
} Node2;

typedef struct List2 {
    Node2 *head;                /* +0x00 */
    Node2 *tail;                /* +0x04 */
    int    count;               /* +0x08 */
} List2;

extern List2 gList020302D0;
extern u8    gRam0202F4D0[];

extern void List2Init(List2 *list);
extern void List2PushFront(List2 *list, Node2 *node);

/* 0x08042790 */
void FUN_08042790(void)
{
    u8 *node;
    s32 left;

    List2Init(&gList020302D0);
    node = gRam0202F4D0;
    left = NODE_COUNT - 1;
    while (left >= 0) {
        List2PushFront(&gList020302D0, (Node2 *)node);
        node += NODE_STRIDE;
        left--;
    }
}
