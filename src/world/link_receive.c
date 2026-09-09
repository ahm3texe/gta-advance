/* Collects the incoming packets — 0x08066798-0x0806686B
 *
 * Swaps the receive buffer and tests each of the four slots.  If the sum of a
 * slot's ten half words is -13 the packet is considered sound: the 16-byte
 * payload is copied into the caller's buffer and that slot's bit is added to
 * the result mask.  The slot is cleared in every case.
 *
 * The -13 constant is certain: BuildLinkPacket (0x0806673C) writes the
 * checksum as `~total - 12`, so the sum including the checksum always comes
 * out as -13.
 *
 * The swap and the flag read are done with IME off; the serial interrupt could
 * change the buffer at exactly that moment.
 *
 * STATUS: PARKED — 204/212 bytes (8 short); the structure and the instruction
 * sequence are the same as the ROM's.
 *
 * The only difference is the register count: the ROM uses THREE high registers
 * (r8 = the global's address, r9 = the destination buffer, sl = the -13
 * constant), we use TWO.  The missing 8 bytes are exactly that third
 * register's save/restore pair.
 *
 * THE FULL BREAKDOWN OF THE 8 BYTES (2026-09-07, aligned instruction by
 * instruction; 95 in the ROM / 89 in ours):
 *   prologue  +1 (`mov r5,r8` and one more register in the push list)
 *   the loop  +1 (`mov r1,r8 / ldr r0,[r1]`; with the global address in r7 for
 *                us, a single `ldr r0,[r7]` suffices)
 *   the loop  +1 (`mov r0,r8 / ldr r1,[r0]` after the first CpuSet)
 *   epilogue  +1 (`pop {r3,r4,r5}` + `mov sl,r5`)
 * Apart from that the WHOLE instruction sequence and every field access are
 * identical.
 *
 * THE CAUSE IS A SINGLE ALLOCATION DIFFERENCE (measured with dump_alloc): the
 * ROM puts `slot` in the CALLEE-SAVED r5, we put it in r1.  The ROM:
 * r4=payload, r5=slot, r6=i, r7=i+1, r8=address, r9=dest, sl=-13 -> seven
 * callee-saved.  Ours: r4=payload, r5=i, r6=i+1, r7=address, r8=dest, r9=-13
 * with slot in r1 -> six.  Our `slot` allocno (p30, refs 6 / lifetime 14 /
 * priority 0.857) DOES NOT CROSS ANY CALL, so `find_reg` gives it the lowest
 * free register (r1).  The ROM's form only arises when `slot` crosses a call.
 * Every source spelling that keeps `slot` live until the second CpuSet is
 * undone by GCSE: `slot + 4` is a common subexpression in both uses, so it is
 * hoisted into a single pseudo (r4) BEFORE the `if` and `slot` dies there.
 *
 * THE PERMUTER WAS RUN (1519 iterations): a baseline score of 1095 -> a best
 * of 195.  But the 195 candidate is SEMANTICALLY WRONG: it uses the `total`
 * variable as both the outer loop's bound and the inner sum, so the bound is
 * corrupted after the first iteration.  Only its correct part (taking the
 * payload into a local inside the `if`) was kept, and on its own it changed
 * nothing.  The rule: a match that is not understood, or that breaks the
 * meaning, is not accepted (docs/WORKFLOW.md 6).
 *
 * SPELLINGS RULED OUT: the counters as s32/u32 (u32 fixed THE INNER LOOP,
 * 29 -> 8 bytes), writing the payload as a variable/expression, taking it into
 * a local inside the if.
 *
 * SPELLINGS RULED OUT (2026-09-07, all 204 bytes / two high registers):
 * deriving `payload = slot + 4` before the if, inside the if and AFTER the if;
 * holding `slot` as a `LinkPacket *` and writing `pk->payload` (both on the
 * fill side alone and on both sides); writing the fill destination through
 * another view such as `(u32 *)slot + 1` / `(u16 *)slot + 2` (GCSE merges them
 * anyway); reading `pending` BEFORE the swap; splitting the swap into two
 * explicit steps (an `other` temporary -- the same code); making `swap` a
 * `u8 *`; putting `zero = 0` before the if; putting the flag `|=` before the
 * copy; taking `dest + i*16` into a local; writing the inner loop as
 * `word[j]` / `*word++` / `(u16 *)slot[j]` / `*(u16 *)(slot + j*2)` / a
 * `while` with an end pointer (208, worse); making `i`/`j` s32; making `total`
 * u32/int; six permutations of the declaration order.
 *
 * A WARNING -- a wrong turn: writing `byte02 |= byte03` and `return byte03`
 * through `block->` brings in the THIRD high register (`push {r5,r6,r7}`) but
 * the size DROPS to 200: on those two lines the ROM loads the global address a
 * THIRD time (`ldr r2,=0x02036338`), so `gRam02036338->` is the right form.
 * The pressure does not come from there.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/link_receive.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "comm_block.h"

#define SLOT_COUNT      4
#define SLOT_STRIDE     24
#define SLOT_HALFWORDS  10
#define PAYLOAD_BYTES   16
#define CHECKSUM_TOTAL  (-13)
#define CPUSET_COPY_16  0x04000004   /* 32-bit, four words */
#define CPUSET_FILL_16  0x05000004   /* fixed source, 32-bit, four words */

extern void CpuSet(const void *src, void *dst, u32 control);

/* 0x08066798 */
u32 ReceiveLinkPackets(u8 *dest)
{
    CommBlock *block;
    void      *swap;
    u8         pending;
    u32        i;
    u32        j;
    s32        total;
    u16       *word;
    u8        *slot;
    s32        zero;

    block = gRam02036338;
    if (block->arrived == 0)
        return 0;

    REG_IME = 0;

    swap            = block->bufEPtr;
    block->bufEPtr  = block->bufDPtr;
    block->bufDPtr  = swap;

    pending        = block->arrived;
    block->arrived = 0;

    REG_IME = 1;

    gRam02036338->byte03 = 0;

    if (pending != 0) {
        for (i = 0; i <= SLOT_COUNT - 1; i++) {
            slot = (u8 *)gRam02036338->bufEPtr + i * SLOT_STRIDE;

            total = 0;
            word = (u16 *)slot;
            for (j = 0; j <= SLOT_HALFWORDS - 1; j++) {
                total += *word;
                word++;
            }

            if ((s16)total == CHECKSUM_TOTAL) {
                CpuSet(slot + 4, dest + i * PAYLOAD_BYTES, CPUSET_COPY_16);
                gRam02036338->byte03 |= 1 << i;
            }

            zero = 0;
            CpuSet(&zero, slot + 4, CPUSET_FILL_16);
        }
    }

    gRam02036338->byte02 |= gRam02036338->byte03;
    return gRam02036338->byte03;
}
