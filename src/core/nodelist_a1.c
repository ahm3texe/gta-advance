/* Find an id in the list and bump its counter, otherwise set up the spare node.
 * 0x080543D0, 126 bytes.
 *
 * The list header is +0x00 head, +0x04 spare node. The list is ORDERED by id:
 * the search stops as soon as the current id PASSES the one being sought.
 *
 *   found     -> the UPPER NIBBLE of the node's +0x0B field is incremented by
 *                one and the node is returned (it behaves like a reference
 *                count)
 *   not found -> if the spare node's id is 0x7FEF (the empty marker), it is
 *                unlinked, set up under the new id and relinked
 *   spare not empty -> 0
 *
 * The upper nibble is read SIGNED with `lsls #24 / asrs #28`, so it is a
 * signed counter. THE FIELD TYPE IS STILL u8: the ROM reads it with `ldrb`;
 * making it s8 makes the compiler fold it into `ldrsb` + `asrs #4`
 * (+6 differences).
 *
 * STATUS: BYTE-MATCHING, 126/126.  (An earlier session took it from 41 to 7;
 * this session closed the remaining 7 bytes with two measurements -- (0) and
 * (1) below.)
 *
 * ----------------------------------------------------------------------
 * MEASURED MECHANISMS (all verified with -da dumps / diff_function)
 * ----------------------------------------------------------------------
 * 0) The found block: BUILD THE ACCUMULATOR FROM THE CONSTANT, COMPUTE THE
 *    SHIFT FIRST  [5 bytes]
 *    ROM: `movs r2,#15 / ands r2,r0 / orrs r2,r1 / strb r2,[r3,#11]` -- that
 *    is, the destination of the two-address `andsi3` is the CONSTANT's
 *    register, not k2's.
 *    In every EXPRESSION form such as `(k2 & 15) | ...`, regmove binds the
 *    destination to k2's pseudo (the 20+ attempts of the previous session all
 *    got stuck on this).
 *    THE LEVER: assign the constant to a local and use a COMPOUND ASSIGNMENT
 *    on top of it --
 *        m = 15;  m &= k2;  m |= t;  cur->kind = m;
 *    Expansion then produces `(set m (and m k2))` right from the start, and
 *    regmove has nothing left to flip. On its own this fixed the direction
 *    (7 -> ...), BUT it broke the order: `movs r2,#15 / ands r2,r0` came out
 *    BEFORE the shift.
 *    The second half: `(hi + 1) << 4` must be computed first, in a SEPARATE
 *    statement (`t`). Together the two reproduce the ROM's instruction order
 *    exactly (12 -> 4 differences).
 *
 * 1) THE r5/r6 SWAP -- SOLUTION: MAKE THE LOOP A REAL do/while  [4 bytes]
 *    The ROM has r6=list, r5=id; written plainly it was the other way round
 *    (~10 bytes). The reason: the global allocator's priority is
 *    floor_log2(refs)*refs/lifetime; list has 5 refs/45 = 0.2222 and id 5
 *    refs/46 = 0.2174. list is processed FIRST and find_reg gives it the
 *    LOWEST free register (r5). id's lifetime is ALWAYS 1 longer than list's:
 *    in the prologue list is copied first (def -1), and call arguments are
 *    always produced in the order r0,r1,r2 (last use +2).
 *    THE LEVER (new, general): when the scan loop is written with `goto scan`,
 *    gcc DOES NOT EMIT the NOTE_INSN_LOOP_BEG/END notes, so flow.c's
 *    `REG_N_REFS += loop_depth` weighting never runs and in-loop references
 *    count as 1. Written as an entry-guarded `do { } while`, the same body
 *    does produce the loop notes: id's TWO `cmp` references inside the loop
 *    gain weight (5 -> 7), floor_log2(7)*7/46 = 0.304 overtakes list's
 *    0.2222, id is processed FIRST and takes r5.
 *    Because list never appears inside the loop, its weight does not change --
 *    which is why the threshold can be crossed at all.
 *    A side benefit: the third argument no longer needs to be read FROM
 *    MEMORY; `InsertSorted(list, spare, id)` now gives `adds r2,r5,#0`
 *    directly (the extra `ldrh r2,[r4,#8]` that the previous solution cost is
 *    gone).
 *    NOTE: the loop form was read from the ROM -- an entry-guarded do/while;
 *    a `while` or `for` moves the test to the front and inverts the branching.
 *
 * 2) `movs r1,#2 / negs r1,r1` -- THE (u8) NARROWING
 *    Written plainly, the compiler reduced this to the single instruction
 *    `subs r1,#4`; 2 bytes shift, the literal pool alignment breaks, and the
 *    ~10 instructions in between move, growing the difference by 20 bytes.
 *    The cause was MEASURED: the transformation is `move2add` after RELOAD; in
 *    the dumps `const_int -4` appears FIRST in .greg and is absent from .lreg.
 *    move2add tracks the constant in a HARD register and, when a new constant
 *    is more expensive (a negative constant = movs+negs, 2 instructions),
 *    derives it with `adds/subs`. BUT the tracking is MODE-sensitive: it does
 *    not fire unless reg_mode is the same.
 *    In the ROM `| 16` is QImode (`*movqi_insn`) and `| 2` is QImode too;
 *    because `& -2` is SImode, the chain breaks. In ours `| 2` was SImode ->
 *    the chain formed and `subs r1,#4` came out. Writing
 *    `k = (u8)(k | 2);` drops that constant to QImode and the ROM's movs/negs
 *    pattern comes out exactly.
 *    (The `subs r1,#11` chain from -2 to -13 is present in the ROM too; do
 *    not touch it.)
 *
 * 3) THE POSITION OF `| 2` -- it must come BEFORE the two ANDs, as in the ROM;
 *    but if it is moved forward without the (u8) narrowing, (2) kicks in.
 *
 * ELIMINATED PATHS (do not retry):
 *   - `k & -2` / `& 0xFFFFFFFE` / `& (0-2)` / taking the masks into locals
 *     (`m = ~1; k &= m;`): all give the same RTL, none breaks the chain.
 *   - Moving `| 2` between the two ANDs or to the end: the chain breaks but
 *     the instruction ORDER diverges from the ROM (15/16 differences; the (u8)
 *     form gives 7).
 *   - `(k | 2) & ~1 & ~12` as a single expression: 122 bytes, the ANDs merge.
 *   - Moving the `spare->slot = 0;` store out from between `| 16` and `| 2`:
 *     the two `orrs` fold into a single `orrs #18`. Its position must stay as
 *     in the ROM.
 *   - Making InsertSorted's third argument `spare->id` and reading it FROM
 *     MEMORY: it fixes r5/r6 but leaves an extra `ldrh r2,[r4,#8]`, so a
 *     4-byte difference remains. The loop lever in (1) replaced this.
 *   - There is NO OTHER way to shorten `id`'s lifetime (argument copies are
 *     always in r0,r1,r2 order); an extra reference such as
 *     `spare->slot = id` also pushes the lifetime to 46.
 *   - The 20+ EXPRESSION forms tried and eliminated for the direction of
 *     `ands r2,r0` in the found block (the solution is not an expression but a
 *     COMPOUND ASSIGNMENT -- see (0)):
 *       `(k2&15) | ((hi+1)<<4)`, `(15&k2)`, `(m15&k2)` (m15 as a local;
 *       declared first or last, assigned inside or outside the block), taking
 *       the result into an s32/u8 local k3, splitting `hi+1` into a separate
 *       instruction, taking the shift into a variable t, RE-READING the field
 *       (`cur->kind`) inside the expression, making k2 u8/u16, and not using
 *       k2 at all. ALL give 7 or worse (14/22/25).
 *     The measured cause: the `regmove` pass ALWAYS binds the destination of
 *     the two-address `andsi3` to k2's pseudo -- in .combine
 *     `(set (reg 81) (and (reg 29) (reg 80)))` and in .regmove
 *     `(set (reg 29) (and (reg 29) (reg 80)))` -- INDEPENDENTLY of operand
 *     order (swapping the operands with m15 gives the same). The ROM binds it
 *     to the constant's register. The solution is not to persuade regmove but
 *     to leave it no work at all: `m = 15; m &= k2;` (see (0)).
 *   - `m |= (hi + 1) << 4;` as one statement: the direction is right but the
 *     order is reversed (`movs #15/ands` comes out before the shift, 12
 *     differences). Taking the shift into a separate `t` statement is
 *     required.
 *
 * Sibling files: nodelist_c3.c already declared this function's signature
 * (`Node *FindOrClaimNode(Node **head, s32 index)`), and nodelist_b6.c calls
 * it. The Node layout was taken from there.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_a1.c
 */

#include "gba_types.h"

#define SPARE_ID 0x7FEF

typedef struct Node {
    struct Node *next;          /* 0x00 */
    u8    pad04[4];
    u16   id;                   /* 0x08 */
    u8    slot;                 /* 0x0A */
    u8    kind;                 /* 0x0B */
} Node;

typedef struct NodeList {
    Node *head;                 /* 0x00 */
    Node *spare;                /* 0x04 */
} NodeList;

extern void ListRemove(NodeList *list, Node *node);
extern void InsertSorted(NodeList *list, Node *node, s32 id);

/* 0x080543D0 */
Node *FindOrClaimNode(NodeList *list, s32 id)
{
    Node *cur;
    Node *spare;
    s32 k;
    s32 cid;
    s32 hi;
    s32 k2;
    s32 m;
    s32 t;

    cur = list->head;
    spare = list->spare;
    /* A REAL loop statement is required: written with `goto`, gcc emits no
     * loop note, id's two in-loop references gain no weight, and
     * r5 is assigned to list -- see header note (1). */
    if (cur != 0) {
        do {
            cid = cur->id;
            if (cid == id) goto found;
            if (cid > id) break;
            cur = cur->next;
        } while (cur != 0);
    }

    if (spare->id != SPARE_ID) goto none;

    ListRemove(list, spare);
    spare->id = id;
    k = (spare->kind & 15) | 16;
    /* The store in between stops `| 16` and `| 2` folding into a
     * single `orrs` (the ROM has two separate `orrs`); the (u8)
     * drops the constant to QImode and breaks the move2add chain
     * -- point (2) in the header. */
    spare->slot = 0;
    k = (u8)(k | 2);
    k = k & ~1;
    k = k & ~12;
    spare->kind = k;
    InsertSorted(list, spare, id);
    return spare;

    /* The ROM keeps this body at the END of the function (`beq` jumps
     * forward); writing it inside the loop moves the block forward and inverts
     * the branching. */
found:
    /* A SEPARATE local: used in both the insert and the found branch, `k`
     * reached 19 references and dropped to r2; split, it falls to 12 and takes
     * the ROM's r0. */
    k2 = cur->kind;
    hi = (s32)(k2 << 24) >> 28;
    /* The shift comes FIRST, in its own statement: the ROM builds
     * `adds #1 / lsls #4` before the mask; written as one expression,
     * `movs #15 / ands` moves ahead of it. */
    t = (hi + 1) << 4;
    /* Build the constant in a local and use a compound assignment ON TOP of
     * it: the destination of the two-address `ands` then becomes the
     * constant's register -- point (0) in the header. */
    m = 15;
    m &= k2;
    m |= t;
    cur->kind = m;
    return cur;

none:
    return 0;
}
