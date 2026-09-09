/* Clear the node and fill the slot arrays from the descriptor — 0x08053930
 * (196 bytes)
 *
 * The flow READ FROM THE ROM:
 *   1. gRam02030C00 = 0  (the mask gate switch; the same symbol as in
 *      nodelist_c3.c and nodelist_d5.c, where it is declared `u32`).
 *   2. Save and clear IME, ZERO the node's 32 bytes (8 words) with DMA3, do a
 *      dead read of DMA3.control, restore IME. The source is a zero local on
 *      the stack, and the control word is 0x85000008 (enable | 32-bit |
 *      src fixed).
 *   3. Find a run of desc->countB consecutive free slots (FindFreeSlotRun,
 *      0x08055D18) -> node->slotsB; if countB is nonzero and the returned
 *      pointer is NULL, the function ends.
 *   4. The same for countA / node->slotsA.
 *   5. Two copy loops: slotsB <- desc->listB (+0x0C), calling PrepareAreaNode
 *      for every id written; slotsA <- desc->listA (+0x08), calling
 *      LinkAreaEntryIfEligible (0x08052C68) for every id written.
 *   6. node->desc = desc (+0x14), node->unk0A = 0xFF (+0x0A).
 *
 * MEASURED 1 — the node is 32 bytes: the lower half of the DMA control word is
 *   8 words, i.e. 0x20 bytes. Since the struct's last field is +0x1C (slotsB),
 *   this is consistent with the layout in nodelist_d3.c; the +0x0A field was
 *   `pad0A` there, 0xFF is written to it here, and it was left as `unk0A` (its
 *   meaning is unknown, and no name was invented).
 *
 * MEASURED 2 — the zero constant is shared in THREE places: the ROM emits a
 *   single `movs r0,#0` and uses it for the gRam02030C00 store, for
 *   `REG_IME = 0` and for the fill local on the stack. For that to come out,
 *   the `fill = 0;` line must be written AFTER `REG_IME = 0;` (the same order
 *   as in the VRAM-clearing block of src/ui/menu_screen.c).
 *
 * MEASURED 3 — the NULL check tests THE RETURN VALUE, not the field:
 *   the ROM copies the return into a separate register with `adds r1,r0,#0`
 *   and does `cmp r1,#0` after the `str`; `node->slotsB` is not re-read. So
 *   the call result is taken into a `slots` local first and the field is
 *   written from that. The count test, on the other hand, IS re-read FROM THE
 *   FIELD (`ldrb r0,[r7,#5]`), because there is a call in between. Rule 34:
 *   both conditions were written as early `return`s.
 *
 * MEASURED 4 — the two tests DO NOT FOLD into a single mask:
 *   `countB != 0 && slots == 0` is over two DIFFERENT variables, so it cannot
 *   enter fold_truthop; the ROM's `cmp #0/beq` + `cmp #0/beq` pair comes out
 *   directly.
 *   (Compare MEASURED 1 in nodelist_d3.c: there two tests were on the same
 *    byte and the source form decided the folding.)
 *
 * MEASURED 5 — THE DECISIVE ONE: each loop gets its OWN source pointer.
 *   With the two loops sharing a single `src` local, the remaining difference
 *   is 14 bytes and ALL of it is an r5/r6 swap: the ROM puts the counter in r5
 *   and the source in r6; with a shared `src` it comes out the other way
 *   round. Making `srcB` and `srcA` separate locals splits the source's
 *   lifetime in two, drops it below the counter in priority, and the
 *   allocation returns to the ROM's: 14 -> 0.
 *   NOTE — this is not the INVERSE of MEASURED 3 in nodelist_d3.c but another
 *   face of the same mechanism: there a lifetime had to be SPLIT, and so it
 *   does here.
 *   Splitting the destination pointer (`dst`) has NO EFFECT; splitting the
 *   counter has NO EFFECT either. What must be split is precisely the SOURCE
 *   pointer.
 *
 * TRIED AND ELIMINATED (all measured; the size is always 196):
 *   - ALL 720 permutations of the declaration order: the difference is always
 *     14. Declaration order changes the allocation not at all in this function
 *     (docs/COMPILER.md's observation that "declaration order on the stack is
 *     not decisive" turned out to hold for register allocation too).
 *   - A separate counter per loop (`i` / `j`): 14. No effect.
 *   - Making the counter block-local to each loop: 14. No effect.
 *   - A separate DESTINATION pointer per loop (`dst` / `dst2`): 14. No effect.
 *   - `i = 0;` before the loops with a `for (; ...)` form: 26 (worse).
 *   - Writing the source assignment before the destination assignment: 24
 *     (worse).
 *   - Making the increment order `i++, src++, dst++`: 16. The ROM's order is
 *     `i++, dst++, src++` (rule 43).
 *
 * Rule 43: the counter and BOTH pointers advance together -> all three in the
 *   `for` increment, in the ROM's order (`adds r5,#1` / `adds r4,#2` /
 *   `adds r6,#2`).
 * Rule 9/31: the counter is `int`; the ROM emits `bge`/`blt` (signed), and the
 *   u8 field is promoted to `int` in the comparison.
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 * Rule 1: 0x02030C00 is an extern symbol (gRam02030C00), not a constant cast.
 *
 * `*dst = *src; PrepareAreaNode(*dst);` — the call argument is re-read from
 * the DESTINATION, not from the SOURCE (`strh r0,[r4]` immediately followed by
 * `ldrh r0,[r4]`). Writing `PrepareAreaNode(*src)` eliminates that second
 * `ldrh`.
 *
 * MATCH: 196/196 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_a6.c
 */

#include "gba_types.h"
#include "gba_io.h"

/* enable | 32-bit unit | source fixed | 8 words = 32 bytes */
#define DMA_CLEAR_NODE  0x85000008

#define NODE_UNK0A_INIT 0xFF

/* The same base as the SlotDesc in nodelist_d3.c and nodelist_d6.c; the
 * +0x08 and +0x0C pointers are additionally read here. */
typedef struct SlotDesc {
    u8   pad00[4];
    u8   countA;                /* +0x04 length of the primary array */
    u8   countB;                /* +0x05 length of the secondary array */
    u8   pad06[2];
    u16 *listA;                 /* +0x08 primary id source */
    u16 *listB;                 /* +0x0C secondary id source */
} SlotDesc;

/* The same layout as the Node in nodelist_d3.c; 32 bytes in total (the DMA
 * clear measures this). */
typedef struct Node {
    u8        pad00[10];
    u8        unk0A;            /* +0x0A */
    u8        pad0B[9];
    SlotDesc *desc;             /* +0x14 */
    u16      *slotsA;           /* +0x18 */
    u16      *slotsB;           /* +0x1C */
} Node;

extern u32 gRam02030C00;

extern u16 *FindFreeSlotRun(int count);
extern void PrepareAreaNode(s32 index);
extern void LinkAreaEntryIfEligible(s32 index);

/* 0x08053930 */
void InitNodeFromDesc(Node *node, SlotDesc *desc)
{
    u16 *dst;
    u16 *srcB;
    u16 *srcA;
    u16 *slots;
    int  i;
    u32  fill;
    u16  ime;

    gRam02030C00 = 0;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = node;
    REG_DMA3.control = DMA_CLEAR_NODE;
    REG_DMA3.control;
    REG_IME = ime;

    slots = FindFreeSlotRun(desc->countB);
    node->slotsB = slots;
    if (desc->countB != 0 && slots == 0)
        return;

    slots = FindFreeSlotRun(desc->countA);
    node->slotsA = slots;
    if (desc->countA != 0 && slots == 0)
        return;

    dst = node->slotsB;
    srcB = desc->listB;
    for (i = 0; i < desc->countB; i++, dst++, srcB++) {
        *dst = *srcB;
        PrepareAreaNode(*dst);
    }

    dst = node->slotsA;
    srcA = desc->listA;
    for (i = 0; i < desc->countA; i++, dst++, srcA++) {
        *dst = *srcA;
        LinkAreaEntryIfEligible(*dst);
    }

    node->desc = desc;
    node->unk0A = NODE_UNK0A_INIT;
}
