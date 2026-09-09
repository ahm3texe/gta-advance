/* Release the entity's linked node and rebuild the link flags
 * 0x08053F6C, 136 bytes (a 4-byte pool at the end and an 8-byte one in the
 * middle).
 *
 * The node at the entity's +0x2C field is taken and:
 *   1) its sub-object (+0x28) is cleared,
 *   2) the "dirty" bit and the level nibble in its +0x0B byte are cleared,
 *   3) if the node's +0x16 link id is neither 0 nor 0x3FF, a node is searched
 *      for under that id on the 0x02035A80 list and, if the one found has
 *      0x1000 in its +0x18 flags, the same bit is written into our node too,
 *   4) THE SEARCH IS REPEATED for the same id: if either our node or the one
 *      found has 0x1000, the +0x18 flag word is masked with 0x1C03FFFF,
 *      otherwise with 0x1C00FFFF.
 *
 * DETAILS MEASURED FROM THE ROM
 * -----------------------------
 * - `pop {r4}; pop {r0}; bx r0` -> a void return type (rule 35).
 * - The prologue `push {r4,lr}`: only ONE callee-saved value crosses the calls
 *   (the node pointer, r4). So the link id does NOT cross the call -- the
 *   second search's argument is RE-READ from memory (`ldrh r0,[r4,#22]`).
 * - The node layout is in the same family as the Node in the siblings
 *   nodelist_a4.c / nodelist_b1.c: +0x0B flag byte, +0x18 flag word, +0x28
 *   sub-object. The +0x16 u16 link id is seen for the first time in this file
 *   (`ldrh`, so u16).
 * - The +0x18 field is unsigned HERE: there are only `& 0x1000` and two masks,
 *   with none of the `bits < 0` gate from b1.c, so it was left `u32`.
 * - The constant 0x1000 is built with `movs #0x80 / lsls #5` rather than from
 *   the pool, and the SAME register is reused immediately afterwards for the
 *   `orrs`; so the `node->bits |= LINK_BIT` form is correct (a separate mask
 *   local is NOT NEEDED).
 * - 0x3FF, 0x1C00FFFF and 0x1C03FFFF are pool constants; 0x3FF is compared
 *   against a u16 (it does not fit an immediate). Rule 44 is NOT NEEDED:
 *   canonicalisation only happens in `<`/`<=` tests, and here it is `==`.
 *
 * WHY THERE IS A SINGLE STRB AT +0x0B (measured)
 * ----------------------------------------------
 * The ROM emits one `ldrb` + two `ands` (-2 and 15) + one `strb`. That is the
 * result of two SEPARATE consecutive bitfield assignments (`dirty = 0;` then
 * `level = 0;`): the second assignment's `ldrb` is removed by CSE and the
 * first's `strb` by dead-store elimination. The mask order gives the source
 * order: -2 (dirty) first, then 15 (level).
 * Furthermore the sequence `movs r0,#0 / str r0,[r4,#40] / subs r0,#2`
 * derives the -2 constant from the preceding zero; so the `node->sub = 0;`
 * line comes BEFORE the bitfield assignments.
 *
 * FORMS I TRIED AND ELIMINATED (all measured)
 * -------------------------------------------
 * 1. A SINGLE shared `other` local (both search results into the same
 *    variable): 136 bytes but 68 differences. The first search's result cannot
 *    stay in r0, an `adds r1,r0,#0` copy appears and the r1/r2 roles in the
 *    tail get swapped too. Rule 45: giving each search its own local
 *    (`linked` / `other`) brings the difference to 0. That the ROM consumes
 *    the first result in r0 and puts the second in r2 is exactly the trace of
 *    this distinction.
 * 2. Passing the local `id` to the second search (`FindFreeNode(id)`): 8
 *    differences. The value crosses the call, the prologue becomes
 *    `push {r4,r5,lr}` and the id is allocated r5; the ROM has NO r5. This is
 *    direct evidence for the claim "the second read comes from memory" -- the
 *    inverse direction of rule 11.
 * 3. `level = 0;` first, `dirty = 0;` second: 90 differences. The masks follow
 *    source order, the first constant becomes `movs #15`, and -2 can no longer
 *    be derived from the zero, so it is built with `movs #2 / negs` (one extra
 *    instruction, shifting every pool offset).
 * 4. Making +0x0B a plain `u8 kind` and writing `kind &= 0x0E` on one line:
 *    132 bytes, 4 SHORT. agbcc folds the two masks into `movs r0,#14`; the
 *    ROM's two separate `ands` only come out from two separate bitfield
 *    assignments.
 *
 * MATCH: 136/136 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_b3.c
 */

#include "gba_types.h"

#define LINK_NONE   0x3FF        /* the "no link" id for +0x16 */
#define LINK_BIT    0x1000       /* the link bit in the +0x18 flag word */
#define KEEP_PLAIN  0x1C00FFFF   /* bits kept when there is no link */
#define KEEP_LINKED 0x1C03FFFF   /* with a link, 0x30000 is kept as well */

/* A node on the 0x02035A80 list; the layout is from the same family as
 * src/core/nodelist_a4.c and src/core/nodelist_b1.c, with +0x16 added here. */
typedef struct Node {
    struct Node *next;          /* +0x00 */
    u8    pad04[7];
    u8    dirty : 1;            /* +0x0B bit 0    */
    u8    pad0B : 3;            /* +0x0B bit 1..3 */
    s8    level : 4;            /* +0x0B bit 4..7 */
    u8    pad0C[10];
    u16   linkId;               /* +0x16 */
    u32   bits;                 /* +0x18 */
    u8    pad1C[12];
    void *sub;                  /* +0x28 */
} Node;

/* The callers (0x0803795C and 0x0803846C) pass this object in r0; only its
 * +0x2C field is used here, and the rest was not named. */
typedef struct Entity {
    u8    pad00[0x2C];
    Node *node;                 /* +0x2C */
} Entity;

extern Node *FindFreeNode(u32 id);      /* 0x08055954: search the list for an ID */

/* 0x08053F6C */
void ResetNodeLinkBits(Entity *ent)
{
    Node *node;
    Node *linked;
    Node *other;
    u32   flags;
    u32   id;

    node = ent->node;
    if (node == 0)
        return;

    node->sub = 0;
    node->dirty = 0;
    node->level = 0;

    id = node->linkId;
    if (id != 0 && id != LINK_NONE) {
        linked = FindFreeNode(id);
        if (linked != 0 && (linked->bits & LINK_BIT))
            node->bits |= LINK_BIT;
    }

    /* The second search re-reads its argument from memory (see the prologue
       note above); the local `id` is NOT USED here. */
    other = FindFreeNode(node->linkId);
    flags = node->bits;
    if ((flags & LINK_BIT) == 0
     && (other == 0 || (other->bits & LINK_BIT) == 0))
        node->bits = flags & KEEP_PLAIN;
    else
        node->bits &= KEEP_LINKED;
}
