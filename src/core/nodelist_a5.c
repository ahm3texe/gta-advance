/* Build the node's slot array and refresh its record - 0x08052BBC-0x08052C65, 170 bytes.
 *
 * STATUS: BYTE-MATCHING (170/170).
 *
 * What it does: it takes the slot count from the node's +0x14 descriptor
 * (SlotDesc), looks for a run of consecutive free slots with FindFreeSlotRun,
 * writes the run into the node's +0x18 field, copies the descriptor's +0x10 id
 * array into that run and calls RebuildAreaEntry for every id, then writes 0xFF
 * to +0x0A and sets the "dirty" bit. If the bit is still set in the tail,
 * FUN_08051d10 is called with the descriptor's +0x14 pointer and its +0x07 byte.
 *
 * The structs and signatures were TAKEN from the siblings, not invented:
 *   The Node layout + the 0x0B bitfield (dirty:1 / pad:3 / level:4 SIGNED)
 *     and SlotDesc.count(+0x06)  -> src/core/nodelist_c2.c (ReleaseNodeSlots)
 *   FindFreeSlotRun(int) -> u16*  -> src/world/slot_table.c
 *   RebuildAreaEntry(s32, u32)    -> src/core/nodelist_b6.c (the signature was
 *     kept VERBATIM; the ROM passes the node pointer as the second argument,
 *     hence the (u32) conversion at the call site - changing the signature
 *     would be a needless risk for check_consistency)
 *   ReleaseNodeSlots(u32, u32)    -> src/core/nodelist_c2.c
 * SlotDesc's +0x07 (mode), +0x10 (ids) and +0x14 (table) fields were measured
 * from this ROM body; +0x14 is a POINTER because FUN_08051d10 tests it against
 * 0 and writes it to a global as a WORD, and +0x07 is `ldrb`.
 *
 * MEASURED DETAILS
 *
 * 1) TWO SEPARATE ZERO VARIABLES (this was the key to the whole thing).
 *    In the prologue the ROM zeroes r9 with `movs r1,#0 / mov r9,r1`; r9 is
 *    NEVER incremented in the loop. The loop's own counter is r7. r9 is read in
 *    two places:
 *      - the loop ENTRY guard   `cmp r9,r1 / bcs` (r1 = desc->count)
 *      - the test after the loop `mov r0,r9 / cmp r0,#0 / beq`
 *    So there are two variables in the source: `n` (r9) and the loop counter
 *    `i` (r7).
 *    Written as `for (i = n; i < desc->count; i++)`, agbcc derives the guard
 *    from n and the body counter from zero - exactly the ROM's duality.
 *    Because n is never assigned, the `if (n != 0)` body (0x08052C3C, the
 *    ReleaseNodeSlots call) is UNREACHABLE; Ghidra therefore said "Removing
 *    unreachable block (ram,0x08052c3c)", DROPPED the block and reduced the
 *    loop guard to `count != 0`. In the ROM the block IS THERE, so the ROM was
 *    taken as the authority, not the draft.
 *
 * 2) THE `node->slots` ASSIGNMENT AND `dst` ARE SEPARATE (2 bytes were here).
 *    ROM: `adds r1,r0,#0 / str r1,[r6,#24]` ... `cmp r1,#0` ...
 *         `adds r4,r1,#0`  <- an extra copy.
 *    That is, the call result is written into the field first, the zero test is
 *    done on the same value, and the walking pointer is built AFTERWARDS.
 *    Taking the result straight into `dst` and writing `node->slots = dst`
 *    destroys that copy.
 *
 * 3) Rule 35: `pop {r0}; bx r0` -> a void return type.
 * 4) Rule 47: the upper nibble of +0x0B is read with `lsls #24 / asrs #28` ->
 *    a SIGNED 4-bit field, and the `level > 0` comparison is signed.
 *    Bit 0 of the same byte, by contrast, is masked plainly with
 *    `movs #1 / ands`.
 * 5) desc->count is RE-READ every iteration (`ldrb [r8,#6]` inside the loop),
 *    so the condition was not taken into a local.
 *
 * FORMS I TRIED AND ELIMINATED
 *   a. A single variable (`i` as both the loop counter and the final test):
 *      150/170 bytes.
 *      agbcc puts `i` in r2 and SPILLS IT TO THE STACK around every `bl`
 *      (`sub sp,#4` + `str/ldr [sp,#0]`); the ROM's `mov r8/mov r9` high
 *      register pair never forms. So what was missing was not an optimization
 *      but ONE MORE VARIABLE IN THE SOURCE (point 1 above).
 *   b. (a) plus two variables, but with the `node->slots = dst` form: 168/170,
 *      the difference being only the copy from point 2 above and the register
 *      allocation derived from it (the ROM uses r2 as a temporary for r8, ours
 *      r0/r1).
 *      Fixing point 2 fixed the register allocation BY ITSELF; no separate
 *      allocation intervention was needed.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_a5.c
 */
#include "gba_types.h"

typedef struct SlotDesc {
    u8    pad00[6];
    u8    count;                /* +0x06 */
    u8    mode;                 /* +0x07 */
    u8    pad08[8];
    u16  *ids;                  /* +0x10 */
    void *table;                /* +0x14 */
} SlotDesc;

typedef struct Node {
    struct Node *next;          /* +0x00 */
    u8           pad04[4];
    u16          id;            /* +0x08 */
    u8           slot;          /* +0x0A */
    u8           dirty : 1;     /* +0x0B bit 0    */
    u8           pad0B : 3;     /* +0x0B bit 1..3 */
    s8           level : 4;     /* +0x0B bit 4..7 */
    u8           pad0C[8];
    SlotDesc    *desc;          /* +0x14 */
    u16         *slots;         /* +0x18 */
} Node;

extern u16  *FindFreeSlotRun(int count);
extern Node *RebuildAreaEntry(s32 index, u32 value);
extern void  ReleaseNodeSlots(u32 id, u32 force);
extern void  FUN_08051d10(Node *node, void *table, u32 mode);

/* 0x08052BBC */
void AllocateNodeSlots(Node *node)
{
    SlotDesc *desc;
    u16 *dst;
    u16 *src;
    u32  i;
    u32  n;

    desc = node->desc;
    n = 0;
    if (node->dirty == 0) {
        if (node->level > 0) {
            node->slots = FindFreeSlotRun(desc->count);
            if (desc->count != 0 && node->slots == 0) return;
            dst = node->slots;
            src = desc->ids;
            for (i = n; i < desc->count; i++, dst++, src++) {
                *dst = *src;
                RebuildAreaEntry(*dst, (u32)node);
            }
            node->slot = 0xFF;
            node->dirty = 1;
        }
        if (node->dirty == 0) return;
    }
    if (n != 0) ReleaseNodeSlots(node->id, 1);
    if (node->dirty != 0) FUN_08051d10(node, desc->table, desc->mode);
}
