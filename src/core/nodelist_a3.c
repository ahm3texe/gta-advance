/* Walk the entry's two id arrays and empty each of them. 0x08052DDC, 154 bytes.
 *
 * If the entry is active (the upper nibble of +0x0B is nonzero), two arrays are
 * walked:
 *   the array at +0x34  -> every id goes to FUN_08055be8
 *   the array at +0x30  -> every id goes to FindOrRecycleNode; if bit 2 of the
 *                          returned node's +0x0B flags is CLEAR and bit 1 IS
 *                          SET, the node's own +0x18 array is walked too and
 *                          every id is passed to DeactivateAreaNode (with 1 as
 *                          the second argument)
 *
 * The array lengths are RE-READ from the record (+0x28) every iteration: the
 * ROM does `ldrb [r9,#6]` before entering the inner loop and again on exit, so
 * the condition was not taken into a variable.
 *
 * After the inner loop the outer loop's counter and walker are restored
 * (`adds r7,r4,#1` / `mov r8,r5`, then `adds r4,r7,#0` / `mov r5,r8`); that
 * shows the increments are written IMMEDIATELY AFTER the call.
 *
 * STATUS: PARKED, the size MATCHES at 154/154, 17 differences (137 of 154 bytes
 * correct).
 *   first draft                                    150/154, 4 bytes short
 *   take the record into a register before the
 *     flag tests                                   154/154, 22 differences
 *   rule 43 (counter+pointer in the `for`
 *     increment)                                   154/154, 22 differences
 *   declare the counters before the pointers       154/154, 17 differences
 *
 * ALL of the remaining 17 bytes are a single swap: the ROM keeps the counter in
 * r4 and the pointer in r5, ours the other way round. Declaration order does
 * NOT flip this -- three forms were tried (assign the counter first, interleave
 * the pointer and counter declarations, reverse i/j), and all three gave 17 or
 * worse. A known register-allocation class.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_a3.c
 */

#include "gba_types.h"

typedef struct Record {
    u8 pad00[5];
    u8 slotCount;               /* +0x05 */
    u8 idCount;                 /* +0x06 */
} Record;

typedef struct Node {
    u8      pad00[0x0b];
    u8      kind;               /* +0x0B */
    u8      pad0C[8];
    Record *record;             /* +0x14 */
    u16    *ids;                /* +0x18 */
} Node;

typedef struct Entry {
    u8      pad00[0x0b];
    u8      kind;               /* +0x0B */
    u8      pad0C[0x1c];
    Record *record;             /* +0x28 */
    u8      pad2C[4];
    u16    *slots;              /* +0x30 */
    u16    *ids;                /* +0x34 */
} Entry;

extern void  FUN_08055be8(u16 id);
extern Node *FindOrRecycleNode(s32 id);
extern void  DeactivateAreaNode(u16 id, s32 flag);

/* 0x08052DDC */
void ReleaseEntryNodeRefs(Entry *entry)
{
    Record *rec;
    Record *rec2;
    Node *node;
    s32 i;
    s32 j;
    u16 *p;
    u16 *p2;
    u16 *q;

    rec = entry->record;
    if ((entry->kind & 0xf0) == 0) return;

    /* Rule 43: the counter and the walker are both in the `for` increment, in
     * the ROM's order (counter first, then pointer). */
    p = entry->ids;
    for (i = 0; i < rec->idCount; i++, p++) {
        FUN_08055be8(*p);
    }

    /* The SECOND loop's pointer must be a SEPARATE local.  Using a single `p`
     * raises refs to 14; because the priority is
     * floor_log2(refs)*refs/lifetime, 3*14/29 = 1.448 overtakes the counter's
     * 1.185 and claims r4.
     * Split, it drops to 2*7/14 = 1.000, the counter is allocated first and
     * takes r4 -- the ROM's layout.  What decides it is not the ratio but
     * floor_log2 dropping by one step. */
    p2 = entry->slots;
    for (i = 0; i < rec->slotCount; i++, p2++) {
        node = FindOrRecycleNode(*p2);
        if (node != 0) {
            /* The ROM takes both into registers BEFORE the flag tests;
             * leaving the record in the loop condition makes it be
             * re-read every iteration. */
            q = node->ids;
            rec2 = node->record;
            if ((node->kind & 2) == 0 && (node->kind & 1) != 0) {
                for (j = 0; j < rec2->idCount; j++, q++) {
                    DeactivateAreaNode(*q, 1);
                }
            }
        }
    }
}
