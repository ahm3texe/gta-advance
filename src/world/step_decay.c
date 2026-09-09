/* StepDecay -- 0x08023974-0x08023A0B (152 bytes) -- MATCHED
 *
 * If it passes three gates it decrements the value and clamps it to the floor;
 * if the peer is flagged it clamps once more with a second threshold; then it
 * does a shifted comparison over a four-entry ROM table and updates the tag.
 *
 * LAYOUT READ OFF THE ROM (the match confirmed it):
 *   r4 = obj, r5 = peer, r1 = delta, r3 = limit (copied to r6 in the scan),
 *   r2 = the gRam address at the gate / the old value in the apply step,
 *   r0 = temporary.
 *   Three callee-saved: r4, r5, r6 -- `push {r4, r5, r6, lr}`.
 *
 * Constant patterns: 0x1000000 = `0x80 << 17`, 0x20000 = `0x80 << 10`
 * (movs+lsls); 0x0063FFFF and 0x0001FFFF come from the pool.
 * Rule 35: `pop {r1}; bx r1` -> r0 carries the return value, signature u32.
 * Arg2 is never read anywhere (r2 is immediately clobbered with the pool
 * address), but since arg3 is node it stays in the signature as a
 * placeholder.
 *
 * THE TWO DIFFERENT-WIDTH READS -- both must be SEPARATE expressions in the
 * source:
 *   gRam02000224: `ldrb` at the gate (0x8023994), `ldr` in the apply step
 *                 (0x80239a8).
 *   obj->stamp   : `ldrb r6,[r4,#12]` at the gate (0x8023996) but
 *                  `str r0,[r4,#12]` in the apply step (0x80239aa). So the
 *                  SAME field is read as a byte and written as a word; writing
 *                  `*(u8 *)&obj->stamp` at the gate is mandatory, a plain
 *                  `obj->stamp` produces `ldr`.
 *
 * THE TWO LEVERS THAT OPENED THE MATCH. Both were pure ALLOCATION issues:
 * control flow and instruction selection were already correct BEFORE them
 * (the size came out at 152 bytes, 65 of the 74 instructions identical), the
 * difference was only in which value landed in which register.
 *
 *  1. RULE 45, on obj->value. The value is loaded in THREE separate places
 *     (0x8023984 gate, 0x80239ac apply, 0x80239e4 scan) and the ROM puts all
 *     three in SEPARATE registers: r0 / r2 / r5. Writing the three loads into
 *     a SINGLE `old` local produced one single pseudo in agbcc
 *     (p28: 9 refs, priority 0.844) and when that pseudo's turn came it took
 *     r3 and pushed limit into the callee-saved r6; from there peer slid into
 *     r2 and the gRam address into r5 in a chain. Three separate locals
 *     (old / cur / val) brought the difference down from 51 to 38 and the
 *     peer, gRam, apply and scan registers ALL fell into place against the
 *     ROM at once.
 *
 *  2. LIVE-RANGE SPLITTING of limit. At the entry to the scan the ROM copies
 *     limit from r3 to r6 with `adds r6, r3, #0`, because r3 is needed for the
 *     table pointer. Written with a single local, agbcc put limit in r6 from
 *     the start and never produced this copy -- that is, we were MISSING one
 *     instruction; the size still coming out as 152 was masked by the
 *     `movs r0, r0` padding added for pool alignment. Writing `base = limit;`
 *     before the scan brings the copy back.
 *
 *     >>> THIS IS THE FIRST KNOWN EXCEPTION to the note "copy-based splitting
 *     is ALWAYS ruled out". It is not ruled out here because there are merge
 *     points BETWEEN the two ranges: `base` is live only in the scan loop,
 *     `limit` only at the gate, no overlap. The earlier rejections
 *     (lst = list) were cases where the ranges were NESTED inside one another
 *     -- the distinguishing criterion is overlap.
 *     Writing `base = obj->limit;` (a second load) matches EXACTLY as well,
 *     because CSE turns the second `ldr` into the same copy. The copy form was
 *     preferred in the source: the ROM has a single `ldr [r4,#4]`, and
 *     writing a second memory read would mislead the reader.
 *
 * SCAN LOOP: in the ROM the table pointer is an induction variable
 * (`adds r3,#4`), not an index -- writing `TABLE[i]` produces an lsls+ldr
 * pair, so a walking pointer is mandatory (rule 37). The increment order is
 * the ROM's: pointer first (0x80239f4), then the counter (0x80239f6). The
 * counter is signed: `cmp r2,#3` + `ble` (rule 31), i.e. `s32 i` and `i <= 3`;
 * a `u32` spelling would give `bls`. The order of the prologue statements is
 * the ROM's too: i, base, val, entry.
 *
 * SPELLINGS TRIED AND RULED OUT (target 152 bytes / 74 instructions):
 *  - First setup (guessed nested conditions): 156 bytes, 145 off.
 *  - The dump_cfg.py goto chain, `span` in a single local: 156 bytes, 145 off,
 *    push {r4,r5,r6,r7,lr} -- one callee-saved too many.
 *  - A single `old` local + walking pointer: 152 bytes, 51 off.
 *  - `TABLE[i]` indexing (do/while): 56 off. The `for` form: 58 off.
 *  - ALL SIX permutations of the prologue statements: all 25 instructions off
 *    -- statement order is NOT the lever here, allocation was the lever.
 *  - `u32 *ram = &gRam02000224;` (taking the address into a local): 53 off.
 *  - Expanding the gate into nested `if`s: 51 off (same output as the &&
 *    chain).
 *  - Changing the order of the local declarations: 51 off (no effect).
 *  - Reassigning to the SAME local with `limit = obj->limit;` (not a split, a
 *    redefinition): 48 off -- the local has to be SEPARATE.
 *  - Placing the `base = limit;` statement AFTER `entry = TABLE;`: 4 off.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/step_decay.c  -> 152/152 matched
 */

#include "gba_types.h"

#define DELTA_MAX   0x0063FFFF
#define PEER_BIT    (0x80 << 17)
#define SPAN_LIMIT  0x0001FFFF
#define SPAN_RESET  (0x80 << 10)
#define TABLE       ((s32 *)0x08342AC8)

typedef struct Obj {
    u8  tag;                    /* +0x00  strb, result of the scan */
    u8  pad01[3];
    s32 limit;                  /* +0x04  used with asrs => signed */
    s32 value;                  /* +0x08 */
    u32 stamp;                  /* +0x0C  read as a byte, written as a word */
} Obj;

typedef struct Peer {
    u8  pad00[24];
    u32 flags;                  /* +0x18 */
} Peer;

typedef struct Node {
    u8    pad00[44];
    Peer *peer;                 /* +0x2C */
} Node;

extern u32 gRam02000224;

/* 0x08023974 */
u32 StepDecay(Obj *obj, s32 delta, u32 unused, Node *node)
{
    Peer *peer;
    s32 limit;
    s32 base;
    s32 old;
    s32 cur;
    s32 val;
    s32 next;
    s32 i;
    s32 *entry;

    peer = 0;
    if (node != 0)
        peer = node->peer;

    if (delta == 0)
        return 0;

    /* Three gates; if all of them pass it says "not its turn yet" and bails. */
    old = obj->value;
    limit = obj->limit;
    if (old < limit && delta <= DELTA_MAX
        && *(u8 *)&gRam02000224 == *(u8 *)&obj->stamp)
        return 0;

    obj->stamp = gRam02000224;

    /* Decrement and clamp to the floor. Zero is already at the floor and is
     * left untouched. */
    cur = obj->value;
    if (cur != 0) {
        next = cur - delta;
        obj->value = next;
        if (next <= 0)
            obj->value = 1;
    }

    /* If the peer is flagged, a second threshold: what falls from the upper
     * region settles on the floor.  `cur` is the value BEFORE the decrement
     * (r2 in the ROM). */
    if (peer != 0 && (peer->flags & PEER_BIT) != 0
        && cur > SPAN_LIMIT && obj->value <= SPAN_LIMIT)
        obj->value = SPAN_RESET;

    /* Shifted threshold scan over the four-entry table; the last one to pass
     * wins.  `base` is limit's scan-time live range: since r3 is needed for
     * the table pointer, the ROM emits the `adds r6, r3, #0` copy here (see
     * header, 2). */
    i = 0;
    base = limit;
    val = obj->value;
    entry = TABLE;
    do {
        if (val <= (base >> *entry))
            obj->tag = i;
        entry++;
        i++;
    } while (i <= 3);

    return 1;
}
