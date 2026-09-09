/* Sorted-node search and ID queries — 0x08055A68-0x08055AF7
 *
 * The node list is sorted by ascending ID: +0 next, +8 u16 ID. Stop and
 * return 0 once the target ID is passed, since it cannot appear later.
 *
 * The fifth function in this cluster (QueryEntity, 0x08055AF8) was separated
 * while non-matching: src/world/entity_query.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/node_search.c
 */

#include "gba_types.h"

#define COORD_KEEP_MASK  0xFFC00000     /* preserve the high 10 bits */
#define COORD_SHIFT      6

typedef struct Node {
    struct Node *next;          /* +0x00 */
    u8           pad04[4];
    u16          id;            /* +0x08 */
} Node;

typedef struct Coords {
    u32 x;                      /* +0x00 */
    u32 y;                      /* +0x04 */
} Coords;

extern Node *gNodeListHead;         /* 0x02035A70 */
extern u32   gRam020272C8;
extern u32   gRam02026F34;
extern u8    gGameState[];

extern u32 FUN_08032548(void);

/* 0x08055A68 — reconstruct the low 22 bits of two coordinates. */
void RandomizeCoords(Coords *coords)
{
    coords->x = (coords->x & COORD_KEEP_MASK) | (FUN_08032548() << COORD_SHIFT);
    coords->y = (coords->y & COORD_KEEP_MASK) | (FUN_08032548() << COORD_SHIFT);
}

/* 0x08055A94 — search for an ID after the supplied node. */
Node *FindNodeAfter(Node *node, int id)
{
    int found;

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

/* 0x08055AA8 — search from the list head. */
Node *FindNode(int id)
{
    Node *node;
    int found;

    node = (Node *)&gNodeListHead;
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

/* 0x08055AC4 */
int ClassifyId(u32 id)
{
    if (gRam020272C8 == id)
        return 1;
    if (gGameState[12] == 0)
        return 0;
    if (gRam02026F34 != id)
        return 0;

    return 2;
}
