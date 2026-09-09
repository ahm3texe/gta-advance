/* Empty the region's two id arrays — 0x08052CF8-0x08052DDB (228 bytes)
 *
 * The descriptor block at the region object's +0x28 field describes two u16 id
 * arrays and a slot block:
 *
 *     +0x05  u8   listA entry count
 *     +0x06  u8   listB entry count
 *     +0x0A  u16  listC slot count
 *
 * If the SIGNED 4-bit upper field ("level") in the region's +0x0B byte is
 * zero, the function returns without doing anything. Otherwise:
 *
 *   1) RefreshThenNotify is called for every id in listB (+0x34).
 *   2) A node is searched for every id in listA (+0x30). If the node exists and
 *      is dirty (bit 0) and its level is 1, ReleaseAreaNode is called for every
 *      id in the node's slot array, the slots are filled with the empty id and
 *      the dirty bit is cleared. If the node exists, FUN_08055D90 is called
 *      with the list head + id IN EVERY CASE.
 *   3) Both arrays are filled with the empty id, and the slot block is cleared
 *      with DMA.
 *
 * This is the array-repeated version of the single-node 0x08055C04
 * (src/core/nodelist_c2.c); the +0x0B bitfields and the inner loop produce
 * byte-for-byte the same code as there.
 *
 * TWO MEASURED DETAILS (each was tried on its own; both are decisive):
 *
 * 1) THE TWO OUTER LOOPS SHARE THE SAME COUNTER. Written with separate `i` and
 *    `j`, agbcc merged the two walking pointers into r4 and pushed the
 *    counters to r5/r6 (22/228 bytes of difference); the ROM does the
 *    opposite: in both loops the counter is r4 and the pointers are r5 and r6.
 *    The reason is allocation priority (the docs/COMPILER.md formula):
 *    separate counters give 8 refs / 26 lifetime -> 0.923, while a shared
 *    counter gives 16 refs / 54 lifetime -> 1.185 and overtakes the pointer.
 *    The floor_log2(16)=4 factor makes merging the references profitable. The
 *    difference dropped from 22 to 8 bytes.
 *
 * 2) DECLARATION ORDER BREAKS AN ALLOCATION TIE. The remaining 8 bytes were
 *    whether the counter or the pointer would be saved into `sl` (a high
 *    register) in the second loop, with the other spilling to the stack. In
 *    the `-dg` dump both increment temporaries carry refs=4 / lifetime=48, so
 *    THEIR PRIORITIES ARE EQUAL; GCC 2.8 breaks the tie by PSEUDO NUMBER, and
 *    the pseudo numbers come from the DECLARATION ORDER in the source
 *    (area=22, desc=23, node=24, ...).
 *    Declared AFTER `entryA`, the counter let the pointer's temporary take the
 *    lower number and claim `sl`. Declaring `i` BEFORE the pointers reversed
 *    the order: `mov sl, r4` (counter) + `str r6, [sp]` (pointer).
 *    8 differences -> 0.
 *
 *    This is the complement of the docs/COMPILER.md measurement "declaration
 *    order does not change the stack layout": it does not change the stack
 *    layout, but it does decide the allocation order among pseudos of EQUAL
 *    PRIORITY.
 *
 * TRIED AND ELIMINATED:
 *   - An `if (node != 0) { ... }` block instead of `continue`: not a single
 *     byte changed (the same RTL).
 *   - Moving the increments out of the `for` clause into the body, right after
 *     the call: it broke the loop rotation and gave much worse code.
 *   - Moving only the `id`/`k` declarations: no effect, because the counter
 *     still comes after the pointers.
 *
 * Other rules applied:
 *   - Rule 19: the `desc` read is written BEFORE the level check; the ROM also
 *     emits `ldr r7, [r0, #40]` before the mask test.
 *   - Rule 24/26: the upper nibble is a SIGNED bitfield; in agbcc the `== 0`
 *     test produces `movs #240 / ands / cmp #0` with no shift.
 *   - Rule 37: the two array walkers are separate locals.
 *   - Rule 43: the counter and the walker are in the `for` increment, in the
 *     ROM's order (`adds r4, #1` first, then `adds r6, #2`).
 *   - Rule 35: `pop {r0}; bx r0` -> a void return type.
 *   - The three calls at the end read the `area->...` fields again rather than
 *     the walkers; so does the ROM.
 *
 * MATCH: 228/228 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_d2.c
 */

#include "gba_types.h"

#define LEVEL_BASE  1

typedef struct SlotDesc {
    u8  pad00[5];
    u8  countA;                 /* +0x05 */
    u8  countB;                 /* +0x06 */
    u8  pad07[3];
    u16 countC;                 /* +0x0A */
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

typedef struct Area {
    u8        pad00[11];
    u8        pad0B  : 4;       /* +0x0B bit 0..3 */
    s8        level  : 4;       /* +0x0B bit 4..7 */
    u8        pad0C[28];
    SlotDesc *desc;             /* +0x28 */
    u8        pad2C[4];
    u16      *listA;            /* +0x30 */
    u16      *listB;            /* +0x34 */
    void     *listC;            /* +0x38 */
} Area;

extern Node *gNodeListHead;         /* 0x02035A70 */

extern Node *FindNode(u32 id);
extern void  RefreshThenNotify(u32 id);
extern void  ReleaseAreaNode(u32 a, u32 b);
extern void  FillSlotsWithNone(void *dest, u32 count);
extern void  ClearSlots(void *dest, u32 count);
extern void  FUN_08055d90(u32 *head, u32 id);

/* 0x08052CF8 */
void ClearAreaIdArrays(Area *area)
{
    SlotDesc *desc;
    Node     *node;
    int       i;                /* the same counter in both outer loops */
    u16      *entryB;
    u16      *entryA;
    u16      *slot;
    u32       id;
    int       k;

    desc = area->desc;
    if (area->level == 0)
        return;

    entryB = area->listB;
    for (i = 0; i < desc->countB; i++, entryB++)
        RefreshThenNotify(*entryB);

    entryA = area->listA;
    for (i = 0; i < desc->countA; i++, entryA++) {
        id = *entryA;
        node = FindNode(id);
        if (node == 0)
            continue;

        if (node->dirty) {
            if (node->level == LEVEL_BASE) {
                slot = node->slots;
                for (k = 0; k < node->desc->countB; k++, slot++)
                    ReleaseAreaNode(*slot, 0);
                FillSlotsWithNone(node->slots, node->desc->countB);
                node->dirty = 0;
            }
        }

        FUN_08055d90((u32 *)&gNodeListHead, id);
    }

    FillSlotsWithNone(area->listB, desc->countB);
    FillSlotsWithNone(area->listA, desc->countA);
    ClearSlots(area->listC, desc->countC);
}
