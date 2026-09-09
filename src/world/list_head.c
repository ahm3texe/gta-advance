/* Linked-list head and chain helpers — 0x08013974-0x080139C9
 *
 * Three leaf functions: search an eight-entry table (32-byte entries), scan
 * a list, and mark nodes along a chain.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/list_head.c
 */

#include "gba_types.h"

#define ENTRY_COUNT   8
#define ENTRY_STRIDE  32
#define CHAIN_LIMIT   15

typedef struct Entry {
    u8 pad00[8];
    struct Entry *next;         /* +0x08 */
} Entry;

typedef struct ListRoot {
    u8     pad00[0x100];
    Entry *listHead;         /* +0x100 */
    Entry *chainHead;        /* +0x104 */
} ListRoot;

extern ListRoot gRam02022E50;
extern u8    gRam02022FA0[];

/* 0x08013974 */
u8 *GetEntryOrDefault(u8 *fallback, u32 which)
{
    u32 idx;

    idx = which - 1;
    if (idx > ENTRY_COUNT - 1)
        return fallback;

    return gRam02022FA0 + (idx << 5);
}

/* 0x0801398C */
s32 CountChain(void)
{
    Entry *node;
    s32 count;

    count = 0;
    node = gRam02022E50.listHead;
    if (node != 0) {
        do {
            node = node->next;
            count++;
            if (node == 0)
                break;
        } while (count <= CHAIN_LIMIT);
    }

    return count;
}

/* 0x080139B0 */
void MarkChain(void)
{
    Entry *node;

    node = gRam02022E50.chainHead;
    if (node == 0)
        return;

    do {
        *((u8 *)node + 3) = 1;
        node = node->next;
    } while (node != 0);
}
