/* Empty the node's two slot arrays — 0x080539F4-0x08053AD7  (228 bytes)
 *
 * The node's descriptor structure (+0x14) carries two separate slot counts:
 * +0x05 is the length of the secondary array (+0x1C) and +0x04 that of the
 * primary array (+0x18). The function walks the secondary array first, then
 * the primary one, and finally fills both with the empty id.
 *
 *   1. The secondary array: for every id a node is obtained with
 *      FindOrInitAreaNode; if the node is dirty AND its level is 1,
 *      FUN_080536BC is called. Then the gRam02035780 list is refreshed with
 *      that id.
 *   2. The primary array: FindNode for every id; if the node exists, is dirty
 *      and has level 1, that node's OWN slot array is emptied (the same pattern
 *      as src/core/nodelist_c2.c: ReleaseAreaNode for every slot, then
 *      FillSlotsWithNone, then clear the dirty bit). If the node was found,
 *      gNodeListHead is refreshed.
 *   3. Both arrays are pulled to the empty id with FillSlotsWithNone.
 *
 * MEASURED 1 — two tests on the same flag byte, TWO DIFFERENT pieces of code:
 *   In the first loop the ROM emits a single mask: `movs #241 / ands / cmp #17`.
 *   In the second loop it emits the same two tests SEPARATELY:
 *   `movs #1 / ands / cmp #0`, then `movs #240 / ands / cmp #16` — sharing a
 *   single `ldrb`.
 *   What decides this is the source FORM: two comparisons joined by `&&` in one
 *   expression are folded into a single mask by agbcc (GCC 2.8.1
 *   fold_truthop), while separate `if` statements are not folded. So the first
 *   loop was written `if (a && b)` and the second with nested `if`s. In
 *   src/core/nodelist_c2.c the second test carries `|| force`, so it was
 *   already unfoldable; the first loop here is the pure example of the folding.
 *
 * MEASURED 2 — the id local must be `u16`, not `u32`:
 *   With `u32 id = *slotB;` we produced the ROM's `ldrh r0,[r5]` +
 *   `adds r4,r0,#0` pair the WRONG WAY ROUND (`ldrh r4` + `adds r0,r4`), a
 *   2-byte difference. Because a `u16` local stays HImode, the call argument
 *   widens into a separate SImode pseudo: the load falls into the argument
 *   register (r0) and the copy that crosses the call into r4.
 *   The local-variable form of rule 15. Both loops were written `u16`; the
 *   arrays' real element type is `u16` anyway.
 *
 * Clearing the dirty bit gives the `movs #2 / negs` pair: ~1 = -2, so the mask
 * stays at 32 bits (rule 26). A bitfield assignment produces this.
 *
 * MEASURED 3 — the loop temporaries must be BLOCK-SCOPED (rule 69):
 *   Making `entry` and `id` shared function-scope locals across both loops
 *   spreads their lifetimes over the whole function; the first loop's `entry`
 *   then wants a callee-saved register too, `desc` is pushed from r7 to r8, and
 *   every `desc->countX` read gains a `mov r2,r8`: 248 bytes / 230 differences.
 *   Giving each loop its own block-scoped `id`/`entry` keeps the first loop's
 *   `entry` short-lived (r1 in the ROM) and settles `desc` into r7.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 * Rule 43: the counter and the pointer advance together -> both in the `for`
 *   increment, in the ROM's order (`adds #1` first, then `adds #2`).
 * Rule 9/31: the counters are `int`; the ROM emits `blt`/`bge` (signed).
 *
 * MATCH: 228/228 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_d3.c
 */

#include "gba_types.h"
#include "node_list.h"

#define LEVEL_BASE  1

/* The descriptor structure carrying the slot counts. Its +0x06 field matches
 * SlotDesc.count in src/core/nodelist_c2.c. */
typedef struct SlotDesc {
    u8 pad00[4];
    u8 countA;                  /* +0x04 length of the primary array */
    u8 countB;                  /* +0x05 length of the secondary array */
    u8 count;                   /* +0x06 the node's own array */
} SlotDesc;

/* The same layout as the Node in src/core/nodelist_c2.c, with +0x1C added. */
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
    u16         *slotsA;        /* +0x18 */
    u16         *slotsB;        /* +0x1C */
} Node;

extern Node *gNodeListHead;         /* 0x02035A70 */

extern Node *FindOrInitAreaNode(u32 id);
extern Node *FindNode(u32 id);
extern void  ClearObjectIdsAndSlots(Node *node);
extern void  ReleaseAreaNode(u32 a, u32 b);
extern void  FillSlotsWithNone(void *dest, u32 count);
extern void  FUN_08055d90(u32 *head, u32 id);

/* 0x080539F4 */
void ClearNodeSlotArrays(Node *node)
{
    SlotDesc *desc;
    u16      *slotB;
    u16      *slotA;
    int       i;

    desc = node->desc;

    slotB = node->slotsB;
    for (i = 0; i < desc->countB; i++, slotB++) {
        u16   id = *slotB;
        Node *entry = FindOrInitAreaNode(id);

        if (entry->dirty && entry->level == LEVEL_BASE)
            ClearObjectIdsAndSlots(entry);
        FUN_08055d90((u32 *)&gRam02035780, id);
    }

    slotA = node->slotsA;
    for (i = 0; i < desc->countA; i++, slotA++) {
        u16   id = *slotA;
        Node *entry = FindNode(id);

        if (entry != 0) {
            if (entry->dirty) {
                if (entry->level == LEVEL_BASE) {
                    u16 *inner = entry->slotsA;
                    int  j;

                    for (j = 0; j < entry->desc->count; j++, inner++)
                        ReleaseAreaNode(*inner, 0);
                    FillSlotsWithNone(entry->slotsA, entry->desc->count);
                    entry->dirty = 0;
                }
            }
            FUN_08055d90((u32 *)&gNodeListHead, id);
        }
    }

    FillSlotsWithNone(node->slotsB, desc->countB);
    FillSlotsWithNone(node->slotsA, desc->countA);
}
