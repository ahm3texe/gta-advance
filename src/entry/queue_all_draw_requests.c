/* Queue a draw request for every node on the chain — 0x0801508C-0x080150B3
 *
 * Walks the +0x44 chain and, for each node that has a +0x1C record, queues it
 * with bit 5 of the +0x20 halfword as the second argument.
 *
 * Rule 33: the 1 is materialised after the shift and the shifted value anded
 * INTO it, so the bit test is `(field >> 5) & 1` with the 1 in its own local.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/queue_all_draw_requests.c
 */

#include "gba_types.h"

#define FLAG_SHIFT  5

typedef struct DrawRecord DrawRecord;

typedef struct DrawNode {
    u8               pad00[0x1C];
    DrawRecord      *record;    /* +0x1C */
    u16              flags;     /* +0x20 */
    u8               pad22[0x22];
    struct DrawNode *next;      /* +0x44 */
} DrawNode;

extern void QueueDrawRequestDirect(DrawRecord *record, u32 flag);

/* 0x0801508C */
void FUN_0801508c(DrawNode *node)
{
    DrawRecord *record;
    u32 flag;

    if (node == 0)
        return;
    do {
        record = node->record;
        if (record != 0) {
            flag = node->flags >> FLAG_SHIFT;
            flag &= 1;
            QueueDrawRequestDirect(record, flag);
        }
        node = node->next;
    } while (node != 0);
}
