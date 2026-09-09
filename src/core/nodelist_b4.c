/* Search the free list for an id and return the end of the chain
 * 0x080546CC-0x08054743  (120 bytes)
 *
 * gList02035A80 (0x02035A80) is a list header: +0 head, +4 tail, +8 count
 * (data/ram_map.csv: the free list of gNodePoolA). The nodes on the list are
 * in ASCENDING id order, and the scan stops once the id is passed -- the same
 * layout as the sibling FindOrRecycleNode (src/core/nodelist_c1.c).
 *
 * The flow:
 *   - id is ID_NONE                                        -> 0
 *   - id is not below the record table's count             -> 0
 *   - not found on the list                                -> 0
 *   - found -> if the +0x0B `ready` bit is set, FUN_080521c4 is called; then,
 *     if the node has an owner (+0x28) and bit 0 of the owner's +0x0C flag is
 *     set, THE NODE ITSELF is returned; otherwise the +0x30 chain is walked to
 *     its end and the last link returned.
 *
 * DETAILS MEASURED FROM THE ROM:
 *
 * 1) The scan loop loads the walking pointer from a SINGLE place: in the ROM
 *    `ldr r0,[r0,#0]` is the same instruction on the first iteration and on
 *    every later one (0x080546F2). So the list header is treated as a Node*,
 *    `cur` is set directly to the header's address and control JUMPS into the
 *    loop body; there is NO separate `list->head` read as in the sibling file.
 *
 * 2) The "found" and "return zero" bodies stand in the source BEFORE the scan,
 *    immediately AFTER the entry checks that consult the table. That is the
 *    ROM's block order: entry -> `return 0` -> pool -> found -> scan. It is
 *    the inverse of rule 49's layout, so the bodies were NOT MOVED to the end.
 *
 * 3) The record table is in the ROM (0x08D49C00), so a constant cast is used
 *    (rule 1 applies to RAM only). This function uses the +0x04 count and the
 *    +0x1C array base; the entry size is 64 bytes (`lsls r0,r2,#6`). The
 *    sibling file uses the +0x24 field of the same table with 36-byte entries
 *    -- separate arrays.
 *
 * 4) The +0x0B flag byte has the same bitfield layout as in the sibling file;
 *    the ROM emits the order `movs r0,#2 / ldrb r1,[r4,#11] / ands r0,r1`,
 *    which is exactly the output of the bitfield form `node->ready != 0`.
 *
 * 5) THE TABLE BASE IS TWO SEPARATE LOCALS (rule 22 + rule 17). Written with a
 *    single `table` variable, agbcc loaded the base straight into r3:
 *      ldr r3,=0x08D49C00 / ldr r0,[r3,#4] / cmp r2,r0
 *    The ROM instead takes the base into r0 first, reads the count, and THEN
 *    copies it to r3:
 *      ldr r0,=0x08D49C00 / ldr r1,[r0,#4] / adds r3,r0,#0 / cmp r2,r1
 *    Assigning the constant to TWO SEPARATE locals (`probe`, `table`) in
 *    separate statements produces that copy: CSE reduces the second load to a
 *    copy, but because two separate lifetimes remain, the short-lived `probe`
 *    stays in r0 and the long-lived `table` in r3. That was the only
 *    difference: 14/120 -> 0/120.
 *    Writing `table = probe;` (copy-based splitting), by contrast, IS
 *    ELIMINATED -- it merges the two variables into one pseudo; re-assigning
 *    FROM THE CONSTANT is required.
 *    The count must be in a separate local (`count`) too, otherwise the
 *    comparison moves after the table read.
 *
 * WHAT I TRIED AND ELIMINATED (measured against the ROM):
 *   - a single `table` local with the constant used directly at the use site:
 *     14 bytes of difference (explained in point 5 above).
 *   - a structured `while (cur != 0)` loop: agbcc peels the first iteration
 *     and the id comparison appears twice in the scan body.
 *   - an early return `if (id >= t->count) return 0;`: the `return 0` block
 *     falls to the end of the function and the entry branches invert.
 *   - short-circuiting the owner check as
 *     `if (owner != 0 && (owner->flags & 1) != 0)`: the bodies merge and the
 *     chain walk moves forward.
 *
 * The inverse of rule 35: `pop {r1}; bx r1` with a value in r0 -> it RETURNS A VALUE.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_b4.c
 */

#include "gba_types.h"

#define ID_NONE     0x7FEF
#define ENTRY_SIZE  64

/* The +0x0B flag byte is split into bitfields (the same as in the sibling
 * nodelist_c1.c). Only `ready` (bit 1) is read in this function; the meaning
 * of the others is unknown and the names are TEMPORARY. */
typedef struct Node {
    struct Node   *next;            /* +0x00 */
    struct Node   *prev;            /* +0x04 */
    u16            key;             /* +0x08 ID */
    u8             unk0A;           /* +0x0A */
    u8             low    : 1;      /* +0x0B bit 0 */
    u8             ready  : 1;      /* +0x0B bit 1 */
    u8             unk0B2 : 1;      /* +0x0B bit 2 */
    u8             unk0B3 : 1;      /* +0x0B bit 3 */
    u8             hi     : 4;      /* +0x0B bit 4-7 */
    u8             pad0C[0x1C];     /* +0x0C..0x27 meaning unknown */
    struct Owner  *owner;           /* +0x28 */
    u8             pad2C[4];        /* +0x2C */
    struct Node   *chain;           /* +0x30 the next link of the chain */
} Node;

/* The node's owner. Only bit 0 of the +0x0C word is read. */
typedef struct Owner {
    u8  pad00[0x0C];                /* +0x00..0x0B */
    u32 flags;                      /* +0x0C */
} Owner;

typedef struct Entry {
    u8 pad00[ENTRY_SIZE];
} Entry;

typedef struct RecordTable {
    u8     pad00[4];                /* +0x00 */
    int    count;                   /* +0x04 upper bound of valid ids */
    u8     pad08[0x14];             /* +0x08..0x1B */
    Entry *entries;                 /* +0x1C */
} RecordTable;

#define RECORD_TABLE  ((const RecordTable *)0x08D49C00)

extern Node *gList02035A80;         /* 0x02035A80 list head */

extern void FUN_080521c4(Node *node, Entry *entry);

/* 0x080546CC */
Node *FindRecordNodeEnd(int id)
{
    const RecordTable *probe;   /* short-lived: the count read-only */
    const RecordTable *table;   /* long-lived: for the +0x1C array base */
    Node  *cur;
    Node  *node;
    Owner *owner;
    int    key;
    int    count;

    if (id == ID_NONE)
        goto none;
    probe = RECORD_TABLE;
    count = probe->count;
    table = RECORD_TABLE;
    if (id < count)
        goto scan;

none:
    return 0;

found:
    node = cur;
    goto check;

scan:
    cur = (Node *)&gList02035A80;

advance:
    cur = cur->next;
    if (cur == 0)
        goto notfound;
    key = cur->key;
    if (key == id)
        goto found;
    if (key <= id)
        goto advance;

notfound:
    node = 0;

check:
    if (node == 0)
        goto none;
    if (node->ready != 0)
        FUN_080521c4(node, table->entries + id);

    owner = node->owner;
    if (owner == 0)
        goto walk;
    if ((owner->flags & 1) != 0)
        goto done;

walk:
    while (node->chain != 0)
        node = node->chain;

done:
    return node;
}
