/* Flush the pending palette transfers — 0x08013900-0x08013973
 *
 * It walks the list at gRam02022E50 +0x104; for every node whose flag is set it
 * waits for DMA3 to drain, starts the palette transfer with interrupts disabled
 * and clears the flag.
 *
 * The control flow was extracted with tools/dump_cfg.py (8 blocks, three merge
 * points) and written with labels.
 *
 * Constant patterns: the +0x104 offset is BUILT with
 * `movs r1,#130 / lsls r1,#1` and the 0x80000000 mask with
 * `movs r1,#128 / lsls r1,#24`; 0x05000200 and 0x84000008 are loaded FROM THE
 * POOL.
 *
 * The wait loop: the ROM first SKIPS the loop with `cmp r0,#0 / bge` if bit 31
 * is already clear, and otherwise spins on the mask.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 *
 * STATUS: PARKED — 97/120 differences (the ROM is 116 bytes).  The structure is
 * right, the allocation is not.
 *
 * WHAT WAS GAINED: the first 10 bytes of the prologue match exactly.  That
 * proved the +0x104 access is a STRUCT MEMBER, not an array: because
 * `ldr rX,[rY,#260]` cannot be encoded (the ceiling for a word load is 124),
 * AGBCC builds 260 with `movs #130 / lsls #1 / adds`.  Writing it as an array
 * plus a constant offset folded that into a single pool constant.
 *
 * THE REMAINING DIFFERENCE — pinning loop invariants.  The ROM pins all three
 * in registers BEFORE the loop:
 *     ldr  r7, =0x04000208      (the REG_IME address)
 *     movs r3, #0 / mov ip, r3  (the constant 0, in a HIGH register)
 *     ldr  r5, =0x040000D4      (the DMA3 base)
 * Our version reproduces all three inside the loop.
 *
 * TWO ELIMINATED PATHS:
 *   1. An array plus a constant offset  -> the offset folded, 85/116.
 *   2. An address-cast macro instead of a declared extern object
 *      (`extern vu16 REG_IME;`)  -> 99/120, WORSE.  The assumption that the
 *      object form would pin the address turned out to be WRONG.
 *
 * This is the "one live value too many" class left open in docs/COMPILER.md.
 * The same class: step_decay.c, try_engage_target.c, unlink_to_free.c.
 * Parked until a generalisable solution is found.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/flush_palette_queue.c
 */

#include "gba_types.h"

#define DMA_BUSY    (0x80 << 24)
#define PAL_BASE    0x05000200
#define PAL_STRIDE  5
#define DMA_CONTROL 0x84000008


typedef struct DmaChannel {
    void       *src;            /* +0x00 */
    void       *dst;            /* +0x04 */
    vu32        control;        /* +0x08 */
} DmaChannel;

#define REG_IME (*(vu16 *)0x04000208)
#define DMA3    ((volatile DmaChannel *)0x040000D4)

/* THE SAME byte layout as the existing Entry in list_head.c; the inner fields
   of its pad00[8] were named here (sizes and offsets unchanged). */
typedef struct Entry {
    u8            index;        /* +0x00 */
    u8            pad01[2];
    u8            pending;      /* +0x03 */
    void         *src;          /* +0x04 */
    struct Entry *next;         /* +0x08 */
} Entry;

/* The +0x104 offset does not fit the `ldr rX,[rY,#imm]` encoding (the ceiling
   for a word load is 124), so AGBCC builds 260 in a separate register: the
   pattern is a STRUCT MEMBER access, not array arithmetic.  The same structure
   of the same symbol is already used (and matched) in list_head.c. */
typedef struct ListRoot {
    u8     pad00[0x100];
    Entry *listHead;            /* +0x100 */
    Entry *chainHead;           /* +0x104 */
} ListRoot;

extern ListRoot gRam02022E50;

/* 0x08013900 */
void FlushPaletteQueue(void)
{
    Entry *slot;
    Entry *next;
    u16   ime;
    void *src;

    slot = gRam02022E50.chainHead;
    if (slot == 0)
        return;

loop:
    next = slot->next;
    if (slot->pending == 0)
        goto step;

    src = slot->src;
    if ((s32)DMA3->control < 0) {
        while (DMA3->control & DMA_BUSY)
            ;
    }

    ime = REG_IME;
    REG_IME = 0;
    DMA3->src = src;
    DMA3->dst = (void *)(PAL_BASE + (slot->index << PAL_STRIDE));
    DMA3->control = DMA_CONTROL;
    DMA3->control;
    REG_IME = ime;
    slot->pending = 0;

step:
    slot = next;
    if (slot != 0)
        goto loop;
}
