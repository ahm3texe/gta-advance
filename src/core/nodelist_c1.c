/* Search the ordered node list for an id / recycle the last node
 * 0x080547E8-0x0805486F  (136 bytes)
 *
 * gNodeListHead (0x02035A70) is a list header: +0 head, +4 tail, +8 count (the
 * same layout as src/core/linked_list.c and src/core/insert_sorted.c). The
 * nodes on the list are in ASCENDING id order, so the scan stops once the id is
 * passed.
 *
 * There are three outcome paths:
 *   - the id was found on the list      -> that node is used
 *   - not found, the LAST node is empty (0x7FEF) -> that node is unlinked and
 *     re-inserted in order under the new id
 *   - not found, the last node is occupied -> the result is 0
 *
 * A shared tail at the end: if the `ready` bit in the node's +0x0B flag byte is
 * set, FUN_08052988 is called. The ROM enters this tail even when the result is
 * 0, so the `node->ready` read can happen through a null pointer; this is the
 * ORIGINAL BEHAVIOR and the source was written accordingly (otherwise the
 * block layout does not match).
 *
 * FOUR MEASURED DETAILS:
 *
 * 1) The loop was written WITH LABELS (rule 40). Every structured
 *    `while`/`for`/`do-while` form was tried: agbcc weights in-loop references
 *    by loop depth, which raises the key value's priority score (2*6/4 = 3.00)
 *    above the walking pointer's (3*12/19 = 1.90) and lets the key claim r0.
 *    The ROM wants the opposite. In the labeled form no loop note is created,
 *    the weighting disappears and the allocation comes out as the ROM's:
 *    walker r0, key r1.
 *
 * 2) The +0x0B flag byte was written as a BITFIELD. Writing the same triple
 *    with mask arithmetic (`f = (f & 0x0F) | 2; f &= ~1;`) makes agbcc fold the
 *    masks, or derive -2 from the existing 2 with `sub r1,r1,#4`. Bitfield
 *    assignments merge into a single ldrb/strb and produce the ROM's
 *    `movs r1,#2 / negs r1,r1` pair.
 *
 * 3) The "found" block was moved to a SEPARATE label (`found:`) in the source
 *    and written AFTER the "not found" block. In the form
 *    `if (key == id) { node = cur; goto check; }`, agbcc inverts the condition
 *    and puts the block directly after the branch (`bne` + fall-through); the
 *    ROM instead keeps the block behind the pool and skips over it with `beq`.
 *    The block order follows the label order in the source.
 *
 * 4) Because the record table is in the ROM (0x08D49C00), it is written as a
 *    constant cast rather than an extern symbol (rule 1 applies to RAM only).
 *    The same table has the same form in src/world/slot_table.c.
 *
 * The inverse of rule 35: `pop {r1}; bx r1` with a value in r0 -> it RETURNS A VALUE.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_c1.c
 */

#include "gba_types.h"

#define ID_NONE       0x7FEF
#define RECORD_SIZE   36

/* The +0x0B flag byte is split into bitfields. Only `ready` (bit 1) and the
 * upper nibble `hi` are used in this function; the meaning of the others is not
 * known yet and the names are TEMPORARY. */
typedef struct Node {
    struct Node *next;          /* +0x00 */
    struct Node *prev;          /* +0x04 */
    u16          key;           /* +0x08 ID */
    u8           unk0A;         /* +0x0A */
    u8           low    : 1;    /* +0x0B bit 0 */
    u8           ready  : 1;    /* +0x0B bit 1 */
    u8           unk0B2 : 1;    /* +0x0B bit 2 */
    u8           unk0B3 : 1;    /* +0x0B bit 3 */
    u8           hi     : 4;    /* +0x0B bit 4-7 */
} Node;

typedef struct List {
    Node *head;                 /* +0x00 */
    Node *tail;                 /* +0x04 */
    u32   count;                /* +0x08 */
} List;

typedef struct Record {
    u8 pad00[RECORD_SIZE];
} Record;

typedef struct RecordTable {
    u8      pad00[0x24];
    Record *records;            /* +0x24 */
} RecordTable;

#define RECORD_TABLE  ((const RecordTable *)0x08D49C00)

extern Node *gNodeListHead;     /* 0x02035A70 */

extern void ListRemove(List *list, Node *node);
extern void InsertSorted(List *list, Node *node, u32 key);
extern void FUN_08052988(Node *node, Record *record);

/* 0x080547E8 */
Node *FindOrRecycleNode(int id)
{
    List *list;
    Node *cur;
    Node *node;
    int   key;

    list = (List *)&gNodeListHead;
    cur = list->head;
    node = list->tail;
    goto test;

advance:
    cur = cur->next;

test:
    if (cur == 0)
        goto notfound;
    key = cur->key;
    if (key == id)
        goto found;
    if (key <= id)
        goto advance;

notfound:
    if (node->key == ID_NONE)
        goto reuse;
    node = 0;
    goto check;

found:
    node = cur;
    goto check;

reuse:
    ListRemove(list, node);
    node->key = id;
    node->hi = 0;
    node->unk0A = 0;
    node->ready = 1;
    node->low = 0;
    InsertSorted(list, node, id);

check:
    if (node->ready != 0)
        FUN_08052988(node, RECORD_TABLE->records + id);

    return node;
}
