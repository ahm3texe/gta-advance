/* Acquire a node for an id and set up its record. 0x08054744, 164 bytes.
 *
 * Same family as FUN_080543D0 (nodelist_a1.c): it searches the ordered list
 * for the id and, failing that, unlinks the spare node (id 0x7FEF) and
 * relinks it under the new id. The difference is that it ends in a SHARED
 * TAIL: if bit 2 of the node's +0x0B flags is set, a pointer to the area
 * record is built (id * 28 + gAreaBank+0x20) and four fields are cleared.
 *
 * THE LOOP FORM DIFFERS FROM THE SIBLING'S. nodelist_a1.c uses the ROM's
 * entry guard plus a do/while that loops from the bottom; here there is a
 * ROTATED `for` (a `b` jumping to the test below). Both are in the same
 * family but were written differently in the source; the shape has to be read
 * from the ROM rather than copied from the sibling.
 *
 * Rule 49: the "found" and "not found" bodies JUMP to the shared tail; they
 * are not written into the flow.
 *
 * The id * 28 multiplication is built with `lsls #3 / subs / lsls #2`
 * (x*8 - x = x*7, then <<2) -- agbcc's constant-multiply pattern.
 *
 * STATUS: MATCHED. 164/164 bytes, 79/79 instructions.
 *   first draft (`for` loop)                  176/164, 162 differences
 *   loop in the ROM's form (explicit jumps)   164/164, 107 -> 80
 *   separate mask local (m) in the tail       80 -> 78, and the tail
 *                                             registers (node r3, k r4)
 *                                             fell exactly into place
 *   final tail mask `n = 3; n = -n;`          78 on its own, 11 together
 *                                             with the insert fix below
 *   insert mask `q = ~1;` as a separate stmt  11
 *   record address as an integer + `bank`     6
 *   writing the insert flag byte as BITFIELDS 6 -> 0   <-- the last step
 *
 * ---------------------------------------------------------------------
 * THE LAST STEP: THE +0x0B FLAG BYTE CANNOT BE WRITTEN WITH MASK ARITHMETIC
 * ---------------------------------------------------------------------
 * The remaining 6 bytes (3 instructions) were the POSITION of the -2 in the
 * insert block:
 *     ROM  : movs r1,#2 / orrs r0,r1 / movs r1,#2 / negs r1,r1 / ands r0,r1
 *     ours : movs r2,#2 / negs r2,r2 / movs r1,#2 / orrs r0,r1 / ands r0,r2
 * That is, the ROM builds -2 AFTER the OR and freshly, in the SAME register
 * (r1) as the 2.
 *
 * WHY MASK ARITHMETIC CANNOT PRODUCE THIS (verified in the compiler source):
 * after reload, `reload_cse_move2add` (gcc/reload1.c) tracks the known
 * constant value of every hard register and rewrites `(set R const)` into
 * `(set R (plus R diff))` when that is cheaper. With Thumb CONST_COSTS,
 * cost(-2, SET) = 3 instructions and cost(-4, PLUS) = 0 -> it ALWAYS folds:
 * while r1 already holds 2, writing -2 collapses to `subs r1,#4`. The same
 * tracker also turns the redundant second `movs r1,#2` (diff 0) into a no-op
 * and deletes it. So "build -2 after the OR" and "keep the movs+negs form"
 * cannot both be satisfied AT SOURCE LEVEL -- only an intervening CODE_LABEL
 * invalidates the tracker, and no such label can be produced from C (tried:
 * `goto mask; mask:` and a bare label; the first jump pass deletes it).
 *
 * SOLUTION: write the flag byte as THREE SEPARATE BITFIELD ASSIGNMENTS. agbcc
 * merges these into a SINGLE ldrb/strb pair and builds the masks in order,
 * each from a FRESH constant; because the "known constant in the same
 * register" state that move2add tracks never arises, the -2 stays as
 * `movs #2 / negs`.
 * MATCHING SIBLING EVIDENCE: src/core/nodelist_c1.c (FindOrRecycleNode) and
 * src/core/nodelist_b5.c (GetOrCreateRecordNode) carry the BYTE-FOR-BYTE same
 * sequence at 0x08054830 and 0x080545C4 in the ROM, and both matched with the
 * bitfield form. Point 2 in the header of nodelist_c1.c had already recorded
 * this trap.
 *
 * TWO VIEWS ARE NEEDED: the tail block reads the same byte with a SINGLE ldrb
 * and uses it both in the `& 2` test and in two masks, then writes it with a
 * SINGLE strb; converting that to bitfields spawns a second ldrb and breaks
 * the match. So `Node.kind` stays a byte (the tail view) while the insert
 * block looks at the same object through `NodeBits` (the bit view).
 *
 * ---------------------------------------------------------------------
 * THREE MECHANISMS MEASURED IN AN EARLIER SESSION (all reusable)
 * ---------------------------------------------------------------------
 *
 * (1) A SEPARATE MASK LOCAL SHIFTS THE REGISTER ALLOCATION  (80 -> 78)
 *     In the tail the ROM kept node in r3 and k in r4; ours gave r2 and r3.
 *     The reason: in the ROM the result of `kind & ~1` is a SEPARATE
 *     block-local pseudo (r2), while ours overwrote k -- so only TWO locals
 *     were live at once in block 10. With a third local, the local allocator
 *     (local-alloc, which runs BEFORE the global one) claims r2, and the
 *     global allocator then gives node r3 and k r4.
 *     IMPORTANT: `m = k & ~1;` IS NOT ENOUGH -- because it is k, regmove
 *     makes it the destination and coalesces the pseudo. For a separate
 *     pseudo the constant must be built FIRST, in its own statement:
 *         m = ~1;   m &= k;
 *     This is the WORKING converse of the rule "copy-based splitting is
 *     always eliminated": if the split starts from the CONSTANT (not from the
 *     value), a new allocno is born.
 *
 * (2) NEGATIVE CONSTANT: BUILD THE CONSTANT FIRST (78 -> 11)
 *     agbcc reduces a negative constant derivable from a live small constant
 *     to a single instruction: with r1=2 live, -2 becomes `subs r1,#4`; with
 *     r1=0 live, -3 becomes `subs r1,#3 / adds r0,r1,#0`. The ROM builds both
 *     FRESH (`movs #2 / negs`, `movs #3 / negs`).
 *     The pass responsible is `reload_cse_move2add`, as explained above; the
 *     remedy is to build the negative constant FIRST and separate its
 *     lifetime from the live constant:
 *         q = ~1;             k = (k | 2) & q;     (-2 is born BEFORE the 2)
 *         n = 3;  n = -n;     m &= n;              (-3 is fresh)
 *     `n = ~2;` as a SINGLE statement IS NOT ENOUGH; the negation must be a
 *     SEPARATE statement.
 *     (In the insert block this remedy was removed -- bitfields took over
 *     there; in the tail it still holds and is still required.)
 *
 * (3) OPERAND ORDER AND LOAD ORDER IN POINTER ARITHMETIC (11 -> 6)
 *     ROM: `ldr r1,[pc]` (=&gAreaBank) FIRST, then the product in r0, then
 *     `ldr r1,[r1,#32]`, then `adds r0,r0,r1`. So the FIRST operand of the
 *     addition is the PRODUCT, and the result shares its register.
 *     Written as `records + id*28`, GCC canonically moves the pointer to the
 *     front; swapping the operands by hand (`id*28 + records`) CHANGES
 *     NOTHING. To break the canonicalization both sides must be made
 *     INTEGERS:  (void *)(id * RECORD_SZ + (s32)bank->records)
 *     -- that fixes the registers but pushes the symbol load AFTER the
 *     product. To restore the load order, the symbol's address is held in a
 *     separate temporary:  bank = &gAreaBank;
 *     TOGETHER the two give both the ROM's order and its registers.
 *
 * ---------------------------------------------------------------------
 * PATHS TRIED AND ELIMINATED (do not retry)
 * ---------------------------------------------------------------------
 *  - `m = k & ~1;` / `m = ~1 & k;` / `m = -2 & k;` / `m = k & 0xFFFFFFFE;`
 *    / `m = (k | 0) & ~1;` -- all four coalesce into one pseudo, 80.
 *  - `n = ~2;` as a single statement (with or without a temporary) -- again
 *    produces `subs r1,#3`, 78.
 *  - `n = 2; n = -n;` -- SEMANTICALLY WRONG (gives ~1, not ~2). It shows a
 *    46-byte difference, but that is coincidental; do not use it.
 *  - EVERY other form of the pointer arithmetic: operand swap, `(id*7)*4`,
 *    `(id*7 << 2)`, `((id<<3)-id)*4`, `&records[id*28]`, a 28-byte AreaRec
 *    array with `&records[id]`, and `off`/`base` temporaries -- apart from
 *    the integer+bank pair in (3), none of them changes the registers; all
 *    stay at 11.
 *  - ALL VALID ORDERINGS of the six statements in the insert block (a
 *    permutation sweep, run twice: from a base of 11 and of 6) -- the old
 *    order is the best; the others are 9 or worse.
 *  - The 576 ORDERINGS of the 8 local declarations (4 pointers! x 4 s32!) --
 *    none gets below 6. Declaration order is NOT a lever in this function.
 *
 *  IN THIS SESSION (attempts to rescue the insert mask with mask arithmetic;
 *  ALL worse than 6, none should be retried):
 *  - `k = (k | 2) & ~1;` as one statement: 78. `k = (k & ~1) | 2;`: 7.
 *  - Every way of splitting the OR and the AND into separate statements (`q`
 *    first, `q` in between, `q = -2`, `q = 1; q = ~q`, `k |= 2; k &= q;`):
 *    160 bytes / 96 differences -- the two `2`s coalesce and `sub r0,r0,#4`
 *    comes out.
 *  - SEPARATE locals for 2 and -2, plus the 10 declaration positions of the
 *    new local (F1/F2 x 10 positions, 20 measurements): all 160/96.
 *  - All pairwise combinations of {q,m,n,t,u} for 2 and -2 (20 measurements):
 *    only `m` (also defined in the tail) prevents the folding, but then m's
 *    allocno spreads across the two blocks and moves to a callee-saved
 *    register (r5), pushing r7 into the push as well: 164 bytes / 42
 *    differences. In the ROM the two masks are in SEPARATE registers (r1 and
 *    r2), so sharing a variable is STRUCTURALLY wrong.
 *  - A second and third assignment to `q` in the same block (`q = 0;
 *    spare->slot = q;` + `q = 2;` + `q = ~1;`): 160/96. A multiply-assigned
 *    pseudo does NOT prevent the folding; what decides it is the reuse of the
 *    same HARD register.
 *  - `q = 2; k = k | q; t = 2; t = -t;` (negation as a separate statement):
 *    the two `2`s merge under CSE, the `neg` survives, but the instruction
 *    count goes to 4 -> 160/96.
 *  - Making `q`/`t`/`k` u8/s8/u16/s16 (16 measurements): best 78.
 *  - Making the `kind` field s8, writing the mask directly on the field with
 *    a compound assignment (`spare->kind &= ~1;`), making `k` u8/s8 (30
 *    measurements): best 21 (`movs #254` folds to one instruction, while the
 *    ROM wants two).
 *  - Opening a block boundary with a label (`goto mask; mask:`, a bare
 *    `mask:`, a second `goto`): the first jump pass deletes the label,
 *    160/96.
 *  - Putting the bitfields inside Node and adding a UNION with an `all`
 *    member: the union alignment pushes the byte from 0x0B to 0x0C, 16
 *    differences. A separate `NodeBits` view is required.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_a2.c
 */

#include "gba_types.h"
#include "node_list.h"

#define SPARE_ID   0x7FEF
#define RECORD_SZ  28
#define INIT_FIELD 0x3FF

typedef struct Node {
    struct Node *next;          /* 0x00 */
    u8    pad04[4];
    u16   id;                   /* 0x08 */
    u8    slot;                 /* 0x0A */
    u8    kind;                 /* 0x0B flag byte, BYTE view */
    u8    pad0C[8];
    u16   init;                 /* 0x14 */
    u8    pad16;
    u8    mark;                 /* 0x17 */
    void *record;               /* 0x18 */
    s32   a;                    /* 0x1C */
    s32   b;                    /* 0x20 */
    s32   c;                    /* 0x24 */
} Node;

/* The BIT view of +0x0B. The same object under a second type; it cannot be
 * placed inside `Node` as a union, because the union alignment pushes the
 * field to 0x0C.
 * The bit names match the sibling src/core/nodelist_c1.c; the meaning of
 * `unk2`/`unk3` is unknown and the names are TEMPORARY. The insert block uses
 * this view: written together, agbcc folds the three into a single ldrb/strb
 * pair and reproduces the ROM's `movs #15 / ands` + `movs #2 / orrs` +
 * `movs #2 / negs / ands` sequence exactly (see "THE LAST STEP" in the
 * header). */
typedef struct NodeBits {
    u8 pad00[11];               /* 0x00..0x0A */
    u8 low   : 1;               /* 0x0B bit 0 */
    u8 ready : 1;               /* 0x0B bit 1 */
    u8 unk2  : 1;               /* 0x0B bit 2 */
    u8 unk3  : 1;               /* 0x0B bit 3 */
    u8 hi    : 4;               /* 0x0B bit 4-7 */
} NodeBits;

/* gRam02035780 is declared in include/node_list.h as `NodeC4 *` (the list
 * HEAD, +0x00).  The spare node is at +0x04; giving the same symbol a second
 * extern type trips check_consistency's `ram-extern` check, so the second word
 * is reached through the shared declaration. */
#define NODE_LIST ((Node **)&gRam02035780)

typedef struct AreaBank {
    u8    pad00[0x20];
    u8   *records;              /* 0x20 */
} AreaBank;

extern AreaBank gAreaBank;
extern void ListRemove(Node **list, Node *node);
extern void InsertSorted(Node **list, Node *node, s32 id);

/* 0x08054744 */
Node *FindOrInitAreaNode(s32 id)
{
    Node **list;
    Node *cur;
    Node *spare;
    Node *node;
    NodeBits *bits;
    s32 k;
    s32 m;
    s32 n;
    AreaBank *bank;

    list = NODE_LIST;           /* the ROM KEEPS the base in r6 */
    cur = list[0];
    spare = list[1];
    goto test;                  /* the ROM jumps to the test below with `b` */

step:
    cur = cur->next;
test:
    if (cur == 0) goto scanned;
    if (cur->id == id) goto found;
    if (cur->id <= id) goto step;

scanned:
    if (spare->id != SPARE_ID) {
        node = 0;
        goto tail;
    }
    goto insert;

    /* Rule 49: in the ROM the `found` block sits IMMEDIATELY after the
     * literal pool, BEFORE the insert body. */
found:
    node = cur;
    goto tail;

insert:
    ListRemove(list, spare);
    spare->id = id;
    /* Flag byte: clear the upper half, set the "ready" bit, clear bit 0.
     * The order is the ROM's instruction order; it cannot be written with
     * mask arithmetic. */
    bits = (NodeBits *)spare;
    bits->hi = 0;
    spare->slot = 0;
    bits->ready = 1;
    bits->low = 0;
    InsertSorted(list, spare, id);
    node = spare;
    goto tail;

tail:
    /* The tail reads the same byte with a SINGLE ldrb and uses it both in
     * the test and in two masks, then writes it with a SINGLE strb -- so here
     * the BYTE view and mask arithmetic are required, NOT bitfields. */
    k = node->kind;
    if ((k & 2) != 0) {
        bank = &gAreaBank;
        node->record = (void *)(id * RECORD_SZ + (s32)bank->records);
        m = ~1;
        m &= k;
        node->mark = 0;
        node->init = INIT_FIELD;
        node->a = 0;
        node->b = 0;
        node->c = 0;
        n = 3;
        n = -n;
        m &= n;
        node->kind = m;
    }
    return node;
}
