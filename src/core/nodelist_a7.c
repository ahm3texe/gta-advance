/* Search the list for a slot and set it up as the "selected node" - 0x08053DF8,
 * 164 bytes.
 *
 * The 0x02035A80 list is walked from the front looking for the first node whose
 * slot (node +0x28) equals the given target. If the sentinel id 0x7FEF or the
 * end of the list is reached first, 0 is returned; if found, the node is
 * written into the binding record's +0x30 field and 1 is returned.
 *
 * Details MEASURED from the ROM
 * -----------------------------
 * - The first parameter (r0) is NEVER used: at 0x08053DFC r0 is immediately
 *   overwritten with a pool constant. It still stands in the signature,
 *   because the target arrives in r1 and the mode in r2. Hence the name
 *   `s32 unused`.
 * - `pop {r4,r5}; pop {r1}; bx r1` -> there IS a return VALUE (r0 is live), so
 *   this is the inverse of rule 35: the return type is NOT void. u32 was
 *   chosen.
 * - Node layout: +0x00 next, +0x08 u16 id, +0x18 flag word (bit 0x400 is
 *   tested), +0x28 slot pointer.
 * - Slot layout: +0x0C flag word (0x10, 0x80, 0x01, 0x40, 0x100).
 * - Binding record: +0x28 slot, +0x30 selected node.
 * - 0x02026F34 and 0x020272C8 each hold a POINTER; the ROM puts the address in
 *   r3 first and reads through it with `ldr r0,[r3,#0]`. Both are declared
 *   `u32` in ram_map (src/world/node_search.c) -- to avoid creating a
 *   CONTRADICTORY extern type, the type was not changed here; the address was
 *   taken and cast.
 *
 * The loop form: writing `for (n = head; n; n = n->next) { ... break; ... }`
 * DIRECTLY produced the ROM's rotated form (entry guard + a `b` to the
 * condition, with the increment at the top of the loop). The sibling 0x08053D48
 * (nodelist_c4.c) is from the same family yet uses a DIFFERENT form (it copies
 * the sentinel into r5 and carries it inside the loop); it was not copied, it
 * was read from the ROM.
 *
 * FORMS TRIED AND ELIMINATED
 * --------------------------
 * 1. `Ctx **base` (without volatile): 160 bytes, 14 differences. agbcc reads
 *    `(*base)` once and keeps it in r3; the ROM instead does a SEPARATE
 *    `ldr r0,[r3,#0]` for the +0x28 and +0x30 accesses. The fix is the narrow
 *    form of rule 39: making the POINTER ITSELF `Ctx *volatile *`. Not making
 *    the field (the Ctx body) volatile -- only this access. 160 -> 164 bytes,
 *    69 -> 10 differences.
 * 2. `if (kind == 2) base = A; else base = B;` (a single local): agbcc loads B
 *    BEFORE THE COMPARISON and conditionally overwrites it with A, and the `b`
 *    in between disappears. With the `b` gone, the pool dump that follows it
 *    disappears too -- the inline `.word 0x02026F34` at 0x08053E58 in the ROM
 *    is exactly the dump behind that `b`. Stuck at 10 differences.
 * 3. The ternary `base = (kind == 2) ? A : B;`: byte-identical code to 2, 10
 *    differences.
 * 4. Two blocks with an explicit `goto` (rule 40) WAS NOT ENOUGH ON ITS OWN:
 *    since the CFG is the same, agbcc hoisted again, 10 differences.
 * 5. RULE 45 SOLVED IT: giving each branch ITS OWN local (`primary` /
 *    `secondary`) and assigning the shared `base` from there prevented the
 *    blocks from merging. 0 differences.
 *    Note: 4 and 5 together also match, so the `goto` is unnecessary; the
 *    structured `if/else` was preferred.
 *
 * MATCH: 164/164 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_a7.c
 */

#include "gba_types.h"

#define SENTINEL_ID 0x7FEF

/* The slot the node points at; only the +0x0C flag word is used here. */
typedef struct Slot {
    u8  pad00[0x0C];
    u32 flags;                  /* +0x0C */
} Slot;

typedef struct ListNode {
    struct ListNode *next;      /* +0x00 */
    u8    pad04[4];
    u16   id;                   /* +0x08 */
    u8    pad0A[0x0E];
    u32   flags;                /* +0x18 */
    u8    pad1C[0x0C];
    Slot *slot;                 /* +0x28 */
} ListNode;

/* The record the pointer inside 0x02026F34 / 0x020272C8 points at. */
typedef struct Ctx {
    u8        pad00[0x28];
    Slot     *slot;             /* +0x28 */
    u8        pad2C[4];
    ListNode *sel;              /* +0x30 */
} Ctx;

extern ListNode *gList02035A80;     /* 0x02035A80 */

/* A SYMBOL DECLARATION THAT IS NEEDED (not written, reported instead): these
 * two stand as `u32` in ram_map, yet here they hold a Ctx pointer. Changing the
 * type would create a contradictory extern with src/world/node_search.c, so the
 * existing type was kept and the address taken. */
extern u32 gRam02026F34;            /* 0x02026F34 */
extern u32 gRam020272C8;            /* 0x020272C8 */

extern void FUN_08041ee4(Slot *slot);

/* 0x08053DF8 */
u32 SelectNodeForSlot(s32 unused, Slot *target, s32 kind)
{
    ListNode *node;
    Ctx *volatile *base;
    Ctx *volatile *primary;
    Ctx *volatile *secondary;
    Slot *slot;

    for (node = gList02035A80; node != 0; node = node->next) {
        if (node->id == SENTINEL_ID)
            break;
        if (node->slot == target)
            break;
    }

    if (node->id == SENTINEL_ID)
        return 0;

    if (node->flags & 0x400) {
        FUN_08041ee4(node->slot);
        node->slot->flags |= 0x10;
    }

    /* Rule 45: a separate local per branch; with one shared local agbcc
       merges the two loads and deletes the `b` in between (and the pool dump
       with it). */
    if (kind == 2) {
        primary = (Ctx *volatile *)&gRam02026F34;
        base = primary;
    } else {
        secondary = (Ctx *volatile *)&gRam020272C8;
        base = secondary;
    }

    slot = (*base)->slot;
    if (slot->flags & 0x80)
        slot->flags &= ~0x80;
    if (slot->flags & 1)
        slot->flags = (slot->flags & ~0x100) | 0x40;
    (*base)->sel = node;
    return 1;
}
