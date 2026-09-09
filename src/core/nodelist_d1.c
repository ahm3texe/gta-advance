/* Empty all of the object's ids and slot records
 * 0x080536BC-0x08053793  (216 bytes; the last 4 are a literal pool)
 *
 * It takes the header (the counters) at the object's +0x18 and the u16 id array
 * at +0x20. For every entry of the id array:
 *   - a node is found from the id (FindNode),
 *   - if the node is "dirty" (bit 0) and its level is 1, ReleaseAreaNode is
 *     called for every id in the node's own slot array, the slots are filled
 *     with the empty id via DMA and the dirty bit is cleared -- this body is
 *     byte-for-byte the same as FUN_08055C04 in nodelist_c2.c,
 *   - if the node exists, FUN_08055D90 is called with the list head + id.
 * Then the object's own id array is filled with the empty id, each of the
 * 60-byte slot records at +0x24 is passed to FUN_08052CF8, the record array is
 * cleared with ClearSlots and the object's dirty bit is cleared.
 *
 * Structure hints:
 *   - The caller RefreshThenNotify (src/world/refresh_then_notify.c) masks the
 *     object's +0x0B byte with 0xF1 and compares it against 17; so the object,
 *     like the nodes, carries bit0 = dirty and bits 4..7 = level.
 *   - The header's +0x05 byte is the id count and its +0x07 byte the slot
 *     record count.
 *   - A slot record is 60 bytes (`adds r5, #60`).
 *
 * MEASURED DETAILS:
 *   - +0x0B is written as a BITFIELD (the same as in nodelist_c1.c /
 *     nodelist_c2.c): the `dirty` test gives `movs #1 / ands / cmp #0`, the
 *     `level == 1` test gives `movs #240 / ands / cmp #16` with no shift, and
 *     `dirty = 0` gives the `movs #2 / negs` pair. Writing mask arithmetic
 *     breaks all three.
 *   - Rule 43: the counter and the pointer advance together -> both in the
 *     `for` increment and in the ROM's order (`adds r4,#1` first, then
 *     `adds r6,#2`). Writing the increments the other way round (`ids++, i++`)
 *     breaks the ROM's order.
 *   - Rule 9/31: the counters are `int`; the ROM's branches are signed
 *     (`bge` / `blt`).
 *   - Rule 11: the id lives across the calls (the ROM keeps it in r9), so it is
 *     taken into a separate local.
 *   - Rule 35: `pop {r0}; bx r0` -> a void return type.
 *   - In the FillSlotsWithNone / ClearSlots calls after the loop, the array
 *     base is RE-READ from the object rather than taken from the advanced
 *     pointer (`ldr r0, [r1, #32]` / `ldr r0, [r2, #36]`).
 *   - `i` is THE SAME variable in both loops. Opening a separate counter for
 *     the second loop raised the difference from 8 bytes to 22.
 *   - `continue` is written for an empty node. Wrapping the body in
 *     `if (node != 0) { ... }` produces the same instructions but breaks the
 *     tie below the wrong way and leaves an 8-byte difference.
 *
 * DECLARATION ORDER MATTERS HERE -- it was the cause of the last 8 bytes:
 *   agbcc's loop pass produces two EXTRA pseudos for the increments carried
 *   across the loop (`i+1` and `ids+2`; 72 and 73 in the -dg dump). Both carry
 *   refs=4 and live_length=48, so their priorities
 *   (floor_log2(4)*4/48 = 0.167) are EXACTLY EQUAL. global.c breaks the tie by
 *   ALLOCNO NUMBER, and that number comes from the DECLARATION ORDER in the
 *   source. If `ids` is declared first, its carrier claims `sl` and the
 *   counter's spills to the stack:
 *       adds r4,#1 / str r4,[sp] / adds r6,#2 / mov sl,r6      (WRONG)
 *   Declaring the counters first reverses the order and the ROM comes out:
 *       adds r4,#1 / mov sl,r4  / adds r6,#2 / str r6,[sp]     (RIGHT)
 *   So the declaration order below cannot be changed; putting `ids` last gives
 *   the same result -- what matters is that `i` is declared BEFORE `ids`.
 *   Because the instructions stay in place and only their STORES are swapped,
 *   this difference closes not through any "rewrite" attempt but only by
 *   reading the -dg dump and seeing the tie.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_d1.c
 */

#include "gba_types.h"

#define LEVEL_BASE  1

typedef struct SlotDesc {
    u8 pad00[6];
    u8 count;                   /* +0x06 */
} SlotDesc;

typedef struct Node {
    struct Node *next;          /* +0x00 */
    u8           pad04[4];
    u16          id;            /* +0x08 */
    u8           pad0A;
    u8           dirty : 1;     /* +0x0B bit 0    */
    u8           pad0B : 3;     /* +0x0B bit 1..3 */
    s8           level : 4;     /* +0x0B bit 4..7 */
    u8           pad0C[8];
    SlotDesc    *desc;          /* +0x14 */
    u16         *slots;         /* +0x18 */
} Node;

/* The object's header: only two counter fields are used. */
typedef struct ObjHeader {
    u8 pad00[5];
    u8 idCount;                 /* +0x05 */
    u8 pad06;
    u8 recordCount;             /* +0x07 */
} ObjHeader;

/* A slot record; only its size (60 bytes) is known. */
typedef struct ObjRecord {
    u8 pad00[60];
} ObjRecord;

typedef struct Obj {
    u8         pad00[11];
    u8         dirty : 1;       /* +0x0B bit 0    */
    u8         pad0B : 3;       /* +0x0B bit 1..3 */
    s8         level : 4;       /* +0x0B bit 4..7 */
    u8         pad0C[12];
    ObjHeader *header;          /* +0x18 */
    u8         pad1C[4];
    u16       *ids;             /* +0x20 */
    ObjRecord *records;         /* +0x24 */
} Obj;

extern Node *gNodeListHead;         /* 0x02035A70 */

extern Node *FindNode(u32 id);
extern void  ReleaseAreaNode(u32 a, u32 b);
extern void  ClearAreaIdArrays(ObjRecord *record);
extern void  FillSlotsWithNone(void *dest, u32 count);
extern void  ClearSlots(void *dest, u32 count);
extern void  FUN_08055d90(u32 *head, u32 id);

/* 0x080536BC */
void ClearObjectIdsAndSlots(Obj *obj)
{
    /* The order matters: the counters must come BEFORE the pointers.
     * See the "DECLARATION ORDER" note in the header. */
    int        i;
    int        j;
    ObjHeader *header;
    ObjRecord *record;
    Node      *node;
    u16       *ids;
    u16       *slot;
    u32        id;

    header = obj->header;
    ids = obj->ids;

    for (i = 0; i < header->idCount; i++, ids++) {
        id = *ids;
        node = FindNode(id);
        if (node == 0)
            continue;

        if (node->dirty) {
            if (node->level == LEVEL_BASE) {
                slot = node->slots;
                for (j = 0; j < node->desc->count; j++, slot++)
                    ReleaseAreaNode(*slot, 0);
                FillSlotsWithNone(node->slots, node->desc->count);
                node->dirty = 0;
            }
        }

        FUN_08055d90((u32 *)&gNodeListHead, id);
    }

    FillSlotsWithNone(obj->ids, header->idCount);

    record = obj->records;
    for (i = 0; i < header->recordCount; i++, record++)
        ClearAreaIdArrays(record);

    ClearSlots(obj->records, header->recordCount);
    obj->dirty = 0;
}
