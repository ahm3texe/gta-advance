/* Set up the area context from a new descriptor -- 0x08053834, 250 bytes.
 *
 * It takes three arguments: the area context, the descriptor structure and a
 * name pointer. The context is first cleared with DMA, then the id
 * corresponding to the name is looked up and written into a single slot, after
 * which the descriptor's id array is copied into the context's slot array and
 * every id is threaded onto the link.
 * Same structure family as its sibling ReleaseObjectNodeRefs
 * (src/core/nodelist_a9.c).
 *
 * STATUS: PARKED, 246/250 (four bytes SHORT), 13 differences -- really a
 *   SINGLE INSTRUCTION.
 *   The previous round was also 246, but with 137 differences and the
 *   eliminated paths WERE NOT WRITTEN DOWN.
 *   This round rule 50 (variable SPLITTING) was applied in two places,
 *   137 -> 13:
 *
 *   1. The two loops shared a single `i` counter; as one allocno its lifetime
 *      grew and the priority ordering came out the inverse of the ROM's
 *      (ROM: first loop counter r4, second loop counter r6, step pointer r5;
 *      ours had both in r5 and the step in r4).  The second loop was given its
 *      own `j`: 33 -> 16 differences, with the allocation identical to the
 *      ROM's.
 *   2. `dst` carried both the FindFreeSlotRun return and the subsequent
 *      `ctx->slots` read; as one allocno it crossed the calls and forced a
 *      callee-saved register (r4).  The ROM keeps the first in r1
 *      (caller-saved).
 *      The first value was taken into a separate `run` local: 16 -> 13.
 *   Both satisfy rule 50's "TWO SEPARATE PRODUCTION SITES" condition
 *   (`i=0` twice / `FindFreeSlotRun()` and `ctx->slots`) and are NOT
 *   copy-based splitting -- which is why they worked.
 *
 * THE ONE REMAINING DIFFERENCE -- measured, mechanism found, NO lever:
 *   ROM:   lsls r0,r0,#16 / lsrs r0,r0,#16 / adds r4,r0,#0 / ldr r0,=0x7FFF
 *   ours:  lsls r0,r0,#16 / lsrs r4,r0,#16 /                 ldr r0,=0x7FFF
 *   The ROM does the u16 narrowing into a TEMPORARY and COPIES it into idx; we
 *   write straight into idx.  2 bytes of copy + 2 bytes of pool alignment
 *   padding = 4.
 *
 *   Traced with RTL dumps (old_agbcc -dr -dc, dumps t.i.rtl / t.i.cse):
 *   agbcc ALREADY PRODUCES the copy during expansion --
 *       (insn 145) reg62 = lshiftrt(reg63,16)     ; zext(raw)
 *       (insn 147) reg29 = reg62                  ; idx = <that temporary>
 *   but insn 147 is ABSENT from t.i.cse: CSE replaces every use of reg29 with
 *   reg62 and drops the dead copy; and even if it were left, combine would
 *   merge the two instructions (reg62 dies in the copy).  For the copy to
 *   survive, either reg62 must still be used AFTER the copy, or there must be
 *   a CSE block boundary (a multiply-preceded label) BETWEEN the copy and
 *   idx's first use.  Neither could be produced from the source (see below).
 *   This has the SAME signature as "Register copy: a class that cannot be
 *   produced at source level" in docs/COMPILER.md (ClearHudFieldA /
 *   ClearHudFieldB, both four bytes short, with an extra `adds rX,rY,#0`).
 *   Do not touch it until a new mechanism turns up.
 *
 * TRIED AND ELIMINATED (~55 variants; 246 bytes / 13 differences unless
 * stated):
 *   - cast forms: (u16)raw, raw & 0xFFFF, ((u32)raw<<16)>>16
 *   - type/declaration: raw as u32/s32/u16, idx as int/unsigned short/register,
 *     swapping the declaration order of idx and raw
 *   - an intermediate temporary: `half = raw; idx = half;` plus the 8
 *     combinations of half/idx in the comparison, the store and the call
 *     argument -- CSE reduces the two to a single register in EVERY case
 *     (rule 50's ban on copies applies here too)
 *   - `raw = (u16)raw; idx = raw;` (narrowing in place)
 *   - comparison forms: ID_NONE != idx, !(idx == ID_NONE),
 *     (idx - ID_NONE) != 0, idx > ID_NONE, (s32)/(u32) casts, taking ID_NONE
 *     into a local (rule 44)
 *   - `(idx ^ ID_NONE) != 0` -> 250 BYTES, 4 differences; `idx < ID_NONE` ->
 *     250 bytes, 8 differences.  The size matches, but the extra instruction
 *     is on the CONSTANT side (`ldr r1,=0x7FFF / adds r0,r1,#0 / cmp r0,r4`)
 *     while the ROM's is on the VALUE side.  The wrong instruction; DO NOT USE
 *     it just to hit the size.
 *   - body forms: assigning idx separately in each branch; a goto chain using
 *     `idx` directly instead of `raw`; init + `break`; a `while` form; two
 *     separate `found` labels; an artificial merge label before the test (with
 *     one and with two predecessors) -- 246..258, all worse
 *   - the 8 combinations of raw instead of idx in the cmp, in `*slot =` and in
 *     the `PrepareAreaNode()` argument (238..246, 16..58 differences)
 *   - a `idx = ID_NONE` pre-assignment at the top of the function: 250 bytes /
 *     18 differences (the constant load moves into the prologue, and the copy
 *     still does not appear)
 *   - `volatile int raw`: 250 bytes / 9 differences (it adds stack traffic)
 *
 * THE ONE PATH NOT TRIED: decomp-permuter (tools/make_permuter_dir.py).
 *   Since the remaining difference is a single reg-reg copy, the score plateau
 *   is narrow; this is the one candidate that fits the "the permuter surfaces
 *   the mechanism, then a programmatic sweep closes it" pattern from memory.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_b2.c
 */
#include "gba_io.h"

#define ID_NONE       0x7FFF
#define DMA_FILL_32   0x85000008

typedef struct AreaName {
    const char *name;
    u8          pad04[24];
} AreaName;

typedef struct AreaBank {
    u8        pad00[8];
    s32       nameCount;
    u8        pad0C[20];
    AreaName *names;
} AreaBank;

typedef struct AreaDesc {
    u8   pad00[4];
    u8   count;
    u8   pad05[3];
    u16 *ids;
} AreaDesc;

typedef struct AreaCtx {
    u8        pad00[10];
    u8        ready;
    u8        pad0B[9];
    AreaDesc *desc;
    u16      *slots;
    u16      *single;
} AreaCtx;

extern AreaBank gAreaBank;
extern u32      gRam02030C00;

extern s32  FUN_0806dd18(const char *a, const char *b);
extern u16 *FindFreeSlotRun(int count);
extern void PrepareAreaNode(s32 id);
extern void LinkAreaEntryIfEligible(s32 index);

/* 0x08053834 */
u32 FUN_08053834(AreaCtx *ctx, AreaDesc *desc, const char *name)
{
    volatile u32 fill;
    u16  ime;
    int  i;
    int  j;                 /* rule 50: the 2nd loop gets its OWN counter */
    int  raw;
    u16  idx;
    u16 *slot;
    u16 *run;               /* rule 50: the slot run's FIRST value kept apart */
    u16 *dst;
    u16 *src;

    gRam02030C00 = 0;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    REG_DMA3.src = (void *)&fill;
    REG_DMA3.dst = ctx;
    REG_DMA3.control = DMA_FILL_32;
    REG_DMA3.control;
    REG_IME = ime;

    if (name == 0) {
        raw = ID_NONE;
        goto found;
    }
    for (i = 0; i < gAreaBank.nameCount; i++) {
        if (FUN_0806dd18(gAreaBank.names[i].name, name) == 0) {
            raw = i;
            goto found;
        }
    }
    raw = ID_NONE;
found:
    idx = raw;

    if (idx != ID_NONE) {
        slot = FindFreeSlotRun(1);
        ctx->single = slot;
        if (slot == 0)
            goto fail;
        *slot = idx;
        PrepareAreaNode(idx);
    }

    /* `run` and `dst` are separate: the ROM keeps the first in caller-saved
     * r1 and the second in r4 because it crosses the calls. Merging them into
     * one local breaks the allocation. */
    run = FindFreeSlotRun(desc->count);
    ctx->slots = run;
    if (desc->count != 0) {
        if (run != 0)
            goto ready;
fail:
        return 0;
    }
ready:

    dst = ctx->slots;
    src = desc->ids;
    for (j = 0; j < desc->count; j++, dst++, src++) {
        *dst = *src;
        LinkAreaEntryIfEligible(*dst);
    }

    ctx->desc = desc;
    ctx->ready = 0xFF;
    return 1;
}
