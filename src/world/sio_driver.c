/* STATUS: PARKED — 2352/2374 bytes, 669/1174 instructions identical (2026-09-07).
 * The previous source: 2356 bytes, 575/1174 instructions. NOT fully
 * byte-matching.
 *
 * THE TX REGISTER CLASH WAS RESOLVED (docs/COMPILER.md, rule 64):
 * PackLocalLinkTag packs the tag of one record from two parallel rings.
 * The pointer is first set to the ring base, then advanced to the requested
 * record.
 * Reducing this to the single expression `entry = base + index` carries THE
 * SAME meaning, but agbcc then keeps the base constant live in r4 between the
 * two tags. Advancing in two steps splits the base into two short-lived
 * pseudos; r4 is freed and the allocation becomes k=r4, tx2=r5, cur=r6, key
 * ring base=r7 (as in the ROM).
 *
 * A controlled measurement (tools/probe_sio_tx.py):
 *   the old direct two expressions        575/1174, 2356 bytes
 *   a helper, entry = base + index        573/1174, 2356 bytes
 *   a helper, entry = base; entry += i    669/1174, 2352 bytes  <- KEPT
 * The `static inline` helper is expanded at both call sites; there is no new
 * BL.
 * A diagnostic candidate that scored 677 through a pure base alias was not
 * taken; `entry`, by contrast, is a single-purpose pointer that really does
 * advance to the indexed record.
 *
 * The symbol in the old handover note was wrong: in the starting build it was
 * gRam020003C0 that held r4, while gRam02000E80 was in r3. Where the first had
 * p1012 (L79, 6 references / 20 lifetime), the kept source has p1005 and p1032
 * (L79, 8 references / 8 lifetime each, r1). k's allocation p44 moved back from
 * r5 to r4. The hypothesis that this would match the whole body was NOT
 * CONFIRMED: the net gain is 94 instructions, and 505 instructions differences
 * remain.
 *
 * WHAT REMAINS:
 * - The old stack copy at 0x08065834 now matches. The first instruction
 *   difference is at 0x0806584C: because the epilogue moved, the inner
 *   branch's target differs.
 * - The RX addresses are (block+constant)+i*16 in the ROM; ours are
 *   (block+i*16)+constant.
 * - There are other allocation and expression differences in the window/queue
 *   blocks.
 * - In the TX tag section the two base loads are now separate, but against the
 *   ROM's r3->r1 / r0->r1 address additions we advance r1 in place.
 * A linear diff aligns the blocks approximately; the score is not proof of an
 * exact match.
 *
 * 2026-09-07 ADDITIONAL MEASUREMENTS (the score did not move; it stayed at 669):
 *
 * 1. THE FIRST REAL DIFFERENCE IS AT 0x08065A52 AND IT IS A CONSEQUENCE, NOT A
 *    CAUSE.
 *    There the ROM jumps into the body with `b.n 0x8065A6A`; the timeout test
 *    (0x08065A5C) has been placed BEFORE the body. The reason is the Thumb
 *    conditional branch range: the LIVE body is about 1232 bytes, and had the
 *    test come after the body the backward branch would have been out of range.
 *    So the layout is a consequence forced by the size of the body's contents;
 *    this block will not align until the body is corrected.
 *
 * 2. THE OUTER LOOP FORM IS NOT A LEVER. Four forms gave BYTE-IDENTICAL output
 *    (2352 bytes, 669/1174): `do {...} while (cond)`, `for (;;) {... if
 *    (!cond) break; }`, `while (1) {... break}`, and an inverted-condition
 *    do/while. agbcc reduces all of them to the same internal form.
 *
 * 3. THE RX ASSOCIATION DOES NOT CLOSE BY REWRITING. For every field the ROM
 *    builds the combined constant (0x190 + the field offset), does
 *    `base + constant`, and then adds `+ i*16` (measured at 0x08065A8E:
 *    movs #207 / lsls #1 / adds / adds). We produce `(base + i*16) + constant`.
 *    Four new forms were tried and ELIMINATED:
 *      FRAME(b)->rx           with array decay to RX_BASE   669 (unchanged)
 *      (&FRAME(b)->rx[0])[i]                                602
 *      FRAME(b)->rx[0 + i]                                  669 (unchanged)
 *      (current) FRAME(b)->rx[i]                            669
 *    The first and third produce byte-for-byte identical output; agbcc folds
 *    them.
 *    Together with those eliminated in the previous round, eight forms have
 *    been tried.
 *
 * EARLIER GAINS ARE PRESERVED:
 * - TX/RX struct members: a u16 store outside the struct was killing the scalar
 *   global reads. The LinkFrame view closed that alias difference (553->568).
 * - A separate tx2 for the second half of the record; separate locals for the
 *   two RX windows (568->575). Reusing blk/tx had dropped it to 569.
 *
 * ELIMINATED (from a base of 575): reversed OR operands 567/568/569;
 * byte/halfword/word tag locals at best 580; index locals at best 586;
 * splitting step and changing the unsigned widths 575.
 * A volatile sweep added accesses the ROM does not have and was not taken.
 * The old RX forms were re-measured against the new base of 669: a u16 access
 * through the field address gives 654; a LinkSlot array/cast view gives 602.
 *
 * Verification:
 *   make c-match FILE=src/world/sio_driver.c
 *   python3 tools/diff_function.py src/world/sio_driver.c FUN_080657d8
 *   python3 tools/dump_alloc.py src/world/sio_driver.c FUN_080657d8 --conflicts
 *   python3 tools/probe_sio_tx.py
 */

/* Main dispatcher of the link (SIO) driver — 0x080657D8-0x0806611D
 *
 * Structure: a single `switch (gVBlankEnabled)` with four states.
 *   0 IDLE     : copies the key state, produces the edge masks, clears them.
 *   1 READY    : handshakes with StepLinkFrame; once (status & 3) == 3 it
 *                extracts the slot number from SIOCNT and moves to LIVE.
 *   2 LIVE     : the main loop -- StepLinkFrame every iteration, processes the
 *                other side's two windows (last and current) into the ring
 *                buffer, walks up to the acknowledgement, then builds and sends
 *                its own record.
 *                Falls to SETTLING if it exceeds 240 frames.
 *   3 SETTLING : the same cleanup as IDLE.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 */

#include "gba_types.h"
#include "gba_io.h"
#include "comm_block.h"
#include "game_state.h"

#define RING_MASK     31
#define WINDOW_HALF   15
#define HOLD_SPAN      6
#define LINK_TIMEOUT 240
#define ACK_NONE      32
#define SLOT_MAGIC   0xDEAD

#define LINK_STATE_IDLE     0
#define LINK_STATE_READY    1
#define LINK_STATE_LIVE     2
#define LINK_STATE_SETTLING 3

/* The frame records inside the communication block.  The record to be sent is
 * at +0x180; the received records start at +0x190, 16 bytes per player. */
#define LINK_TX_OFS 0x180
#define LINK_RX_OFS 0x190

/* The fields of the frame record. Access goes THROUGH THE BLOCK BASE: because
 * the offsets (0x180..0x19F) do not fit Thumb's strh immediate field (0..62),
 * the ROM builds the constant in a register on every access and adds it to the
 * base. Introducing an intermediate `LinkSlot *` variable makes the base
 * address be computed once and produces `strh [r,#2]` -- which is why every
 * access is written as `FRAME(b)->tx.field`.
 *
 * Writing the fields as STRUCT MEMBERS is required: in gcc 2.x's alias
 * analysis, a write inside a struct does NOT invalidate a read of a scalar
 * global outside it (gRam0200048C), whereas a plain `*(u16 *)` write does --
 * and then the `gRam0200048C & 31` that the ROM computes once was being
 * computed twice for us. */
typedef struct LinkSlot {
    u16 index;                          /* +0x00 */
    u16 keys;                           /* +0x02 */
    u16 ack;                            /* +0x04 */
    u16 endIndex;                       /* +0x06 */
    u16 endKeys;                        /* +0x08 */
    u16 tag;                            /* +0x0A */
    u16 endTag;                         /* +0x0C */
    u16 magic;                          /* +0x0E */
} LinkSlot;

typedef struct LinkFrame {
    u8       pad000[LINK_TX_OFS];
    LinkSlot tx;                        /* +0x180 */
    LinkSlot rx[2];                     /* +0x190 */
} LinkFrame;

#define FRAME(b)      ((LinkFrame *)(b))

/* The other side's record for this frame. The ROM RE-READS both the block
 * pointer and the slot number on every access; the macro preserves that. */
#define RX_SLOT       (FRAME(gRam02036338)->rx[gRam020004A4])


extern u16 gVBlankEnabled;              /* 0x02000D08 */
extern u16 gRam0200048C;
extern u16 gRam02000498;

/* The three words in IWRAM are contiguous; the ROM derives one from another
 * with `adds #8` / `adds #4`, so THE DIFFERENCE BETWEEN THEM IS KNOWN at
 * compile time. Written as symbols that derivation does not happen, hence the
 * address constants. */
#define gRam03000098 (*(u32 *)0x03000098)
#define gRam0300009C (*(u32 *)0x0300009C)
#define gRam030000A0 (*(u32 *)0x030000A0)
extern u16 gRam02000230[];
extern u16 gRam02000420[];
extern u8  gRam02000100[];
extern u8  gRam02000140[];
extern u8  gRam020003C0[];
extern u8  gRam02000400[];
extern u8  gRam02000E80[];
extern u32 gRam020110B8;
extern u32 gFrameCounterLate;
extern u8  gRam02036328;
extern s16 gSlotSelector;               /* 0x02000D40 */

/* The addresses of these symbols are resolved from data/ram_map.csv. The
 * symbol references are kept; absolute addresses can produce spurious CSE
 * derivations. */

extern u16 gRam02000134;
extern u16 gRam02000288;
extern u16 gRam020003EC;
extern s16 gRam020004A4;
extern u16 gRam020004C0;
extern u16 gRam02000D0C;
extern u32 gRam02036324;

extern u32  __umodsi3(u32 counter, u32 kind);
extern void MaybeReset(void);
extern void PollInput(void);
extern u32  StepLinkFrame(u8 *dest);
extern void MaybeSetCommByte6(void);
extern void BuildLinkPacket(const void *payload);

/* The low byte carries the step tag and the high byte the window start.
 * See the measurement above for the ring cursor's two-step advance. */
static inline u16 PackLocalLinkTag(u32 index)
{
    const u8 *entry;

    index &= RING_MASK;
    entry = gRam020003C0;
    entry += index;
    return *entry | (gRam02000E80[index] << 8);
}

/* 0x080657D8 */
void FUN_080657d8(u32 mode)
{
    CommBlock *blk;
    /* A SEPARATE pointer for the record's second half (the end*
     * fields): the ROM keeps the first half in one register (r3)
     * and the second in another (r5), so there are two variables
     * in the source. Using a single variable keeps both live and
     * shifts the allocation by one register. */
    CommBlock *tx2;
    CommBlock *tx;
    u32 status;
    u32 id;
    u32 step;
    u32 same;
    s32 base;
    s32 cur;
    s32 end;
    s32 end2;    /* Part 2 uses its own locals (rule 59) */
    s32 hi;
    s32 hi2;
    s32 start;
    s32 start2;
    s32 prev;
    s32 prev2;
    s32 i;
    s32 i2;
    s32 j;
    s32 j2;
    s32 k;
    s32 startTime;
    u8 *slot;
    u16 prevA;
    u16 prevB;
    u16 keysA;
    u16 keysB;

    if (mode != LINK_STATE_LIVE) {
        gGameState.word00++;
        if (gVBlankEnabled != 0 && mode == 0
                && __umodsi3(gGameState.word00, 5) != 0) {
            gRam0300009C = gRam03000098 = mode;
            gRam02036330.half00 = mode;
            gRam02000498 = gRam020003EC = gRam02000D0C = gRam02000134 = mode;
            gRam030000A0 = gRam020004C0 = mode;
            MaybeReset();
            return;
        }
        PollInput();
    }

    switch (gVBlankEnabled) {
    case LINK_STATE_IDLE:
        gGameState.pad06  = gGameState.pressed;
        gGameState.half04 = gGameState.held;
        gRam03000098 = gGameState.half04 & ~gRam0300009C;
        gRam030000A0 = gRam0300009C & ~gGameState.half04;
        gRam0300009C = gGameState.half04;
        gRam02036330.half00 = 0;
        gRam02036330.half02 = 0;
        gRam02000498 = gRam0300009C;
        gRam02000D0C = gRam03000098;
        gRam020004C0 = gRam020003EC = gRam02000134 = 0;
        break;

    case LINK_STATE_READY:
        gRam02036324 = StepLinkFrame((u8 *)FRAME(gRam02036338)->rx);
        MaybeSetCommByte6();
        if ((gRam02036324 & 3) == 3) {
            id = (REG_SIOCNT32 << 26) >> 30;
            if (id <= 1) {
                gSlotSelector  = id;
                gRam020004A4   = 1 - id;
                gVBlankEnabled = LINK_STATE_LIVE;
                gRam02000288   = 2;
                gGameState.word00 = 1;
            }
        }
        gGameState.pad06  = gGameState.pressed;
        gGameState.half04 = gGameState.held;
        gRam03000098 = gGameState.half04 & ~gRam0300009C;
        gRam030000A0 = gRam0300009C & ~gGameState.half04;
        gRam0300009C = gGameState.half04;
        gRam02036330.half00 = 0;
        gRam02036330.half02 = 0;

        tx = gRam02036338;
        FRAME(tx)->tx.index    = 0;
        FRAME(tx)->tx.keys     = 0;
        FRAME(tx)->tx.ack      = ACK_NONE;
        FRAME(tx)->tx.endIndex = 0;
        FRAME(tx)->tx.endKeys  = 0;
        BuildLinkPacket(&FRAME(tx)->tx);

        gRam02000498 = gRam0300009C;
        gRam02000D0C = gRam03000098;
        gRam020004C0 = gRam020003EC = gRam02000134 = 0;
        break;

    case LINK_STATE_LIVE:
        k = gRam0200048C & RING_MASK;
        if (mode == LINK_STATE_LIVE)
            base = k;
        else
            base = (gRam0200048C - 1) & RING_MASK;

        cur = base;
        while (gRam02000100[cur] != 0)
            cur = (cur + 1) & RING_MASK;

        startTime = gFrameCounterLate;
        slot = &gRam02000100[base];

        do {
            status = StepLinkFrame((u8 *)FRAME(gRam02036338)->rx);
            gRam02036324 = status;
            if (((1 << gRam020004A4) & status) != 0
                    && RX_SLOT.magic == SLOT_MAGIC) {
                /* Part 1: the peer's "last" window. */
                if (RX_SLOT.index != RX_SLOT.endIndex) {
                    end = RX_SLOT.endIndex & RING_MASK;
                    if (((end - cur) & RING_MASK) <= WINDOW_HALF) {
                        hi = (RX_SLOT.endTag >> 8) & RING_MASK;
                        if (((hi - cur) & RING_MASK) <= ((end - cur) & RING_MASK)) {
                            start = hi;
                            prev = (cur - 1) & RING_MASK;
                            if (((gRam02000400[prev] + 1) & RING_MASK)
                                    == (RX_SLOT.endTag & RING_MASK)) {
                                i = cur;
                                if (i != hi) {
                                    do {
                                        gRam02000230[i] = gRam02000230[prev];
                                        gRam02000100[i] = 1;
                                        gRam02000400[i] = gRam02000400[prev];
                                        gRam02000140[i] = gRam02000140[prev];
                                        i = (i + 1) & RING_MASK;
                                    } while (i != hi);
                                }
                            }
                        } else {
                            start = cur;
                        }

                        j = start;
                        for (;;) {
                            gRam02000230[j] = RX_SLOT.endKeys;
                            gRam02000100[j] = 2;
                            gRam02000400[j] = RX_SLOT.endTag;
                            gRam02000140[j] = RX_SLOT.endTag >> 8;
                            if (j == end)
                                break;
                            j = (j + 1) & RING_MASK;
                        }

                        while (gRam02000100[cur] != 0)
                            cur = (cur + 1) & RING_MASK;
                    }
                }

                /* Section 2: the other side's window for this frame. */
                end2 = RX_SLOT.index & RING_MASK;
                if (((end2 - cur) & RING_MASK) <= WINDOW_HALF) {
                    hi2 = (RX_SLOT.tag >> 8) & RING_MASK;
                    if (((hi2 - cur) & RING_MASK) <= ((end2 - cur) & RING_MASK)) {
                        start2 = hi2;
                        prev2 = (cur - 1) & RING_MASK;
                        if (((gRam02000400[prev2] + 1) & RING_MASK)
                                == (RX_SLOT.tag & RING_MASK)) {
                            i2 = cur;
                            if (i2 != hi2) {
                                do {
                                    gRam02000230[i2] = gRam02000230[prev2];
                                    gRam02000100[i2] = 3;
                                    gRam02000400[i2] = gRam02000400[prev2];
                                    gRam02000140[i2] = gRam02000140[prev2];
                                    i2 = (i2 + 1) & RING_MASK;
                                } while (i2 != hi2);
                            }
                        }
                    } else {
                        start2 = cur;
                    }

                    j2 = start2;
                    for (;;) {
                        gRam02000230[j2] = RX_SLOT.keys;
                        gRam02000100[j2] = 4;
                        gRam02000400[j2] = RX_SLOT.tag;
                        gRam02000140[j2] = RX_SLOT.tag >> 8;
                        if (j2 == end2)
                            break;
                        j2 = (j2 + 1) & RING_MASK;
                    }

                    while (gRam02000100[cur] != 0)
                        cur = (cur + 1) & RING_MASK;
                }

                /* Part 3: walk forward from the point the peer acknowledged. */
                if (RX_SLOT.ack == ACK_NONE) {
                    k = gRam0200048C & RING_MASK;
                } else {
                    k = RX_SLOT.ack & RING_MASK;
                    if (((gRam0200048C - k) & RING_MASK) > WINDOW_HALF) {
                        k = gRam0200048C & RING_MASK;
                    } else {
                        step = gRam020003C0[k & RING_MASK];
                        same = (step == gRam020003C0[(k - 1) & RING_MASK]);
                        while (k != (s32)(gRam0200048C & RING_MASK)) {
                            if (same
                                    && gRam020003C0[(k + 1) & RING_MASK] - step > 1)
                                break;
                            if (gRam020003C0[k & RING_MASK]
                                    != gRam020003C0[(k + 1) & RING_MASK])
                                break;
                            k = (k + 1) & RING_MASK;
                        }
                    }
                }
            }

            /* If the local side has caught up with the acknowledgement, search
               backwards for the stable band. */
            if ((k & RING_MASK) == (s32)(gRam0200048C & RING_MASK)) {
                step = gRam020003C0[k & RING_MASK];
                for (;;) {
                    k = (k - 1) & RING_MASK;
                    if (gRam020003C0[k] != step)
                        break;
                    if (((gRam0200048C - k) & RING_MASK) > HOLD_SPAN) {
                        k = gRam0200048C & RING_MASK;
                        break;
                    }
                }
            }

            blk = gRam02036338;
            if (blk->ready == 0) {
                FRAME(blk)->tx.index = gRam0200048C & RING_MASK;
                FRAME(blk)->tx.keys  = gRam02000420[gRam0200048C & RING_MASK];
                if (*slot != 0 || mode == LINK_STATE_LIVE)
                    FRAME(blk)->tx.ack = ACK_NONE;
                else
                    FRAME(blk)->tx.ack = cur;

                tx2 = gRam02036338;
                FRAME(tx2)->tx.endIndex = k & RING_MASK;
                FRAME(tx2)->tx.endKeys  = gRam02000420[k & RING_MASK];
                FRAME(tx2)->tx.tag = PackLocalLinkTag(gRam0200048C);
                FRAME(tx2)->tx.endTag = PackLocalLinkTag(k);
                FRAME(tx2)->tx.magic = SLOT_MAGIC;
                BuildLinkPacket(&FRAME(tx2)->tx);
            }

            if (mode == LINK_STATE_LIVE)
                return;

            if (*slot != 0 || gRam0200048C <= 1) {
                if (gRam0200048C > 1) {
                    prevA = gGameState.half04;
                    prevB = gRam02036330.half02;
                    keysB = gRam02000230[base];
                    gRam02036330.half02 = keysB;
                    gRam02036330.half00 = (prevB ^ keysB) & keysB;
                    keysA = gRam02000420[base];
                    gGameState.half04 = keysA;
                    gGameState.pad06  = (prevA ^ keysA) & keysA;

                    if (gSlotSelector == 0) {
                        gRam03000098 = gGameState.pad06;
                        gRam030000A0 = gRam0300009C & ~gGameState.half04;
                        gRam0300009C = gGameState.half04;
                        gRam02000134 = gRam02036330.half00;
                        gRam020004C0 = gRam020003EC & ~keysB;
                        gRam020003EC = keysB;
                    } else {
                        gRam03000098 = gRam02036330.half00;
                        gRam030000A0 = gRam0300009C & ~gRam02036330.half02;
                        gRam0300009C = gRam02036330.half02;
                        gRam02000134 = gGameState.pad06;
                        gRam020004C0 = gRam020003EC & ~keysA;
                        gRam020003EC = keysA;
                    }
                } else {
                    gRam02036330.half02 = 0;
                    gGameState.half04 = 0;
                    gRam0300009C = 0;
                    gRam03000098 = 0;
                    gRam020003EC = 0;
                    gRam02000134 = 0;
                }

                gRam020110B8 += (gRam02036330.half02 + gGameState.half04)
                                << (gRam0200048C & 15);
                gRam02000498 = gRam0300009C | gRam020003EC;
                gRam02000D0C = gRam03000098 | gRam02000134;
                gRam02000100[base] = 0;
                return;
            }
        } while ((s32)(gFrameCounterLate - startTime) <= LINK_TIMEOUT);

        gVBlankEnabled = LINK_STATE_SETTLING;
        gRam02036328 = 1;
        break;

    case LINK_STATE_SETTLING:
        gGameState.pad06  = gGameState.pressed;
        gGameState.half04 = gGameState.held;
        gRam03000098 = gGameState.half04 & ~gRam0300009C;
        gRam030000A0 = gRam0300009C & ~gGameState.half04;
        gRam0300009C = gGameState.half04;
        gRam02036330.half00 = 0;
        gRam02036330.half02 = 0;
        gRam02000498 = gRam0300009C;
        gRam02000D0C = gRam03000098;
        gRam020004C0 = gRam020003EC = gRam02000134 = 0;
        break;
    }
}
