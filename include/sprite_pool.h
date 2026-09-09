/* Shared sprite-node memory layout established from the ROM. */
#ifndef GUARD_SPRITE_POOL_H
#define GUARD_SPRITE_POOL_H

#include "gba_types.h"

/* 0x0201CEC0: a pool of 128 16-byte OAM nodes, followed by list metadata.
 * The layout was established from ROM code at 0x08012B20, 0x08012B9C, and
 * 0x08012C74. The priority field is the secondary sort key for nodes with
 * equal values of `attr2 & 0x0C00`.
 */
#define NODE_COUNT 128

typedef struct Node {
    u16 attr0;                  /* +0x00 */
    u16 attr1;                  /* +0x02 */
    u16 attr2;                  /* +0x04 */
    u8 priority;                /* +0x06 */
    u8 pad07;
    struct Node *next;          /* +0x08 */
    struct Node *prev;          /* +0x0C */
} Node;

typedef struct NodePool {
    Node nodes[NODE_COUNT];     /* +0x000 */
    Node *freeHead;             /* +0x800 */
    Node *activeHead;           /* +0x804 */
    s32 prevCount;              /* +0x808 */
} NodePool;

extern NodePool gNodePool;

#endif /* GUARD_SPRITE_POOL_H */
