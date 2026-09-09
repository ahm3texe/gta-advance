/* Walk the object's id array and slot records and empty each of them.
 * 0x08053794, 158 bytes (NO literal pool).
 *
 * The flow read from the ROM:
 *   header = obj->header            (obj +0x18; the ROM keeps it in r9)
 *   if bit 2 of obj->kind IS SET, it returns without doing anything
 *   it loops over obj->ids (+0x20) for header->idCount (+0x05) iterations:
 *       node = FindOrRecycleNode(*ids)
 *       if the node exists: node->ids (+0x18) and node->record (+0x14) are
 *       taken INTO REGISTERS, after which, if bit 2 of kind is CLEAR and bit 1
 *       IS SET, the node's own id array is walked for record->idCount (+0x06)
 *       iterations and every id is passed to DeactivateAreaNode (with 1 as the
 *       second argument)
 *   then it loops over obj->records (+0x24) for header->recordCount (+0x07)
 *   iterations and passes each 60-byte record to ReleaseEntryNodeRefs
 *
 * This is BYTE-FOR-BYTE the same body as the inner loop of
 * ReleaseEntryNodeRefs in nodelist_a3.c; the struct views were taken from there
 * (Node +0x0B kind, +0x14 record, +0x18 ids; Record +0x06 idCount). The object
 * views (ObjHeader, ObjRecord) were taken from nodelist_d1.c.
 *
 * MEASURED DETAILS:
 *   - NOT THE SAME as the sibling ClearObjectIdsAndSlots (nodelist_d1.c): there
 *     +0x0B was written as a bitfield (the dirty test and the level == 1 test),
 *     whereas here the ROM does two separate mask tests (movs #2/ands and
 *     movs #1/ands, with a single ldrb) -- so the `u8 kind` view from
 *     nodelist_a3.c is the correct one.
 *   - node->ids and node->record are loaded BEFORE the flag tests
 *     (`ldr r5,[r0,#24]` / `ldr r6,[r0,#20]` come before the tests).
 *     MEASURED: moving both inside the `if` body gives 160 bytes / 47
 *     differences -- the loads slide into the inner loop's preheader and an
 *     extra register move appears.
 *   - Rule 43: the counter and the pointer are both in the `for` increment, in
 *     the ROM's order (counter first, then pointer).
 *   - Rule 9/31: the counters are `int`; the ROM's branches are signed
 *     (`bge`/`blt`).
 *   - Rule 35: `pop {r0}; bx r0` -> a void return type.
 *   - The counters are declared BEFORE the pointers (see the DECLARATION ORDER
 *     note in nodelist_d1.c): in the ROM the loop carrier `i+1` takes r7 and
 *     `ids+2` takes r8, so the counter's is allocated first.
 *     MEASURED: declaring the pointers first gives 158 bytes / 10 differences
 *     (r7 and r8 swap, and the size stays the same).
 *   - The outer loop's first test is ONLY AT THE BOTTOM in the ROM (a `b` jumps
 *     to the test), while the second and third loops have an entry guard with a
 *     bottom-looping form. A plain `for` produces all three correctly; the
 *     explicit `goto test` form from rule 49 is NOT NEEDED -- it was not tried,
 *     because the plain form already matched exactly.
 *
 * TRIED AND ELIMINATED:
 *   - `if (node == 0) continue;` instead of `if (node != 0) { ... }` -- BOTH
 *     match and give the same instructions. (In nodelist_d1.c it was the other
 *     way round; `continue` was required there. In this loop it makes no
 *     difference.)
 *   - Reverting either of the two measurements above (the load position, the
 *     declaration order) reopens the difference; no third lever was needed.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_a9.c
 */

#include "gba_types.h"

typedef struct Record {
    u8 pad00[6];
    u8 idCount;                 /* +0x06 */
} Record;

typedef struct Node {
    u8      pad00[0x0b];
    u8      kind;               /* +0x0B */
    u8      pad0C[8];
    Record *record;             /* +0x14 */
    u16    *ids;                /* +0x18 */
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
    u8         pad00[0x0b];
    u8         kind;            /* +0x0B */
    u8         pad0C[12];
    ObjHeader *header;          /* +0x18 */
    u8         pad1C[4];
    u16       *ids;             /* +0x20 */
    ObjRecord *records;         /* +0x24 */
} Obj;

extern Node *FindOrRecycleNode(s32 id);
extern void  DeactivateAreaNode(u16 id, s32 flag);
extern void  ReleaseEntryNodeRefs(ObjRecord *record);

/* 0x08053794 */
void ReleaseObjectNodeRefs(Obj *obj)
{
    /* The order matters: the counters must come BEFORE the pointers. */
    int        i;
    int        j;
    ObjHeader *header;
    ObjRecord *record;
    Node      *node;
    Record    *rec;
    u16       *ids;
    u16       *slot;

    header = obj->header;
    if ((obj->kind & 2) != 0)
        return;

    ids = obj->ids;
    for (i = 0; i < header->idCount; i++, ids++) {
        node = FindOrRecycleNode(*ids);
        if (node != 0) {
            slot = node->ids;
            rec = node->record;
            if ((node->kind & 2) == 0 && (node->kind & 1) != 0) {
                for (j = 0; j < rec->idCount; j++, slot++)
                    DeactivateAreaNode(*slot, 1);
            }
        }
    }

    record = obj->records;
    for (i = 0; i < header->recordCount; i++, record++)
        ReleaseEntryNodeRefs(record);
}
