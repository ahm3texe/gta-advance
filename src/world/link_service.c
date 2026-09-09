/* Link frame service — 0x0806686C-0x08066903
 *
 * If the block is ready, it swaps the send/receive buffers and starts a new
 * multiplayer transfer: it records SIOCNT's error bit, writes the word to be
 * sent, starts the transfer, and turns Timer3 on together with its interrupt
 * for the timeout.
 *
 * If the block is not ready it waits up to four frames; after the fourth it
 * raises the BIOS interrupt flag and gives up. This rare path sits at the
 * END of the function in the ROM (rule 49), which is why it is the `else`
 * branch in the source as well.
 *
 * SIOCNT is read 32 BITS wide here: the error bit is fetched together with
 * the upper halfword by a single `ldr`, then extracted with a shift 25 left
 * + 31 right.
 *
 * STATUS: PARKED — 144/152 bytes. The instruction sequence is identical to
 * the ROM's; the remaining difference is that the ROM keeps the global
 * address in r4 (callee-saved) while we keep it in r3. The missing 8 bytes
 * are that push/pop pair and its alignment:
 *   ROM `push {r4,lr}` (2) + `pop {r4} / pop {r0} / bx r0` (6) = 8
 *   us   NO push       (0) + `bx lr`                        (2) = 2
 *   on top of that, because the ROM's instruction body is 2 bytes longer,
 *   the first pool takes 2 bytes of padding for 4-byte alignment.  The
 *   59-instruction body is identical.
 *
 * SIO_PORT (the NON-volatile view) is the CORRECT form: in the ROM there is
 * NO read before the `send` write, whereas the volatile REG_SIO in gba_io.h
 * produces a dead `ldrh`. This form was measured at 0x08066904.
 *
 * RULE 57 APPLIES HERE TOO — THE EARLIER "no change" NOTE WAS WRONG
 * (fixed 2026-09-07): reading and writing `gBiosIrqFlags` through
 * `*(volatile u16 *)&...` does not change the SIZE (144) but it makes the
 * `else` branch line up exactly with the ROM's form.  The non-volatile
 * spelling loads the constant first
 * (`ldr r1,=flags / mov r0,#0x80 / ldrh r3,[r1] / orr r0,r3 / strh r0,[r1]`);
 * the volatile view gives the ROM's order
 * (`ldr r2,=flags / ldrh r0,[r2] / mov r1,#0x80 / orr r0,r1 / strh r0,[r2]`)
 * and it also pushes the REG_IME address into r3 the way the ROM does.  So
 * the criterion must not be the byte count alone, but the instruction
 * sequence.  In this state the only places left that DIVERGE from the ROM
 * are the prologue/epilogue and the registers of the `block`/swap
 * temporaries.
 *
 * ALLOCATION DIAGNOSIS (dump_alloc): the pseudo for the global address (p25,
 * refs 3 / lifetime 70 / priority 0.043) is the LAST allocno of the global
 * allocator; since r0-r3 are free it takes r3.  For it to land in r4 as in
 * the ROM, all four would have to be occupied.  The key point is the SWAP
 * TEMPORARY in block L4: the ROM puts it in r3, we put it in r1.  Every
 * spelling that can push the temporary into r3 also shifts `block` from r2
 * to r3 at the same time, so none of them fits exactly.
 *
 * RULED OUT: read-modify-write the control through SIO_PORT.control (no
 * change), taking the error bit through control instead of 32 bits (148
 * bytes, but the ROM does a 32-bit `ldr` -- wrong form).
 *
 * RULED-OUT SPELLINGS (2026-09-07, all 144 bytes / NO push): a second
 * `CommBlock *live` local (inside the block and at the start of the block);
 * taking byte0 into a local with `u8 flag` (rule 55); a `u8 retry` local;
 * writing the outer test as `gRam02036338->byte0`; a `SioPort *sio` local;
 * taking errorBit into an intermediate variable; nested `if` instead of
 * `&&`; writing the else branch through the global; three separate region
 * pointers (rule 59); splitting the swap into two explicit steps; making the
 * two temporaries a single `void *` (block shifts to r1); making both of
 * them `LinkPacket *` / `void *`; doing the packet swap first; moving
 * `recvLen = -1` after the swaps; writing TM3 before SIO_START; the six
 * permutations of the notification order; dropping the `block` local
 * entirely and writing everything through `gRam02036338->`.
 *
 * TWO SPELLINGS REACH 152 BYTES but their instruction ORDER does not match
 * the ROM's, so they were NOT TAKEN: (1) hoisting the reads of both swaps to
 * the front and gathering their writes at the back (`push {r4,lr}` appears,
 * 19/152 bytes off) and (2) writing `ready = 0` before the swaps (39/152).
 * Both raise the pressure by one and push the address into r4, but they
 * shift `block` from r2 to r3; in the ROM `block` is in r2 and the swap
 * temporary is in r3.  So what is missing is not "one more live value", but
 * the temporary being in r3 while `block` stays in r2.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/link_service.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "comm_block.h"

/* NON-volatile view of SIOCNT + SIOMLT_SEND. Writing `send` through the
 * volatile SioRegs in gba_io.h makes agbcc emit a dead `ldrh`; the
 * non-volatile view leaves only the `strh`.
 * Measured at 0x08066904. */
typedef struct SioPort {
    u16 control;                /* +0x00 */
    u16 send;                   /* +0x02 */
} SioPort;

#define SIO_PORT (*(SioPort *)REG_SIOCNT_ADDR)

#define RETRY_LIMIT     3
#define SIO_ERROR_SHIFT 25          /* moves bit 6 to the top of the 32 bits */
#define SIO_START       0x0080
#define SIO_SEND_IDLE   0xFEFE
#define TM3_ON_WITH_IRQ 0x00C0
#define BIOS_IRQ_SERIAL 0x0080

/* 0x0806686C */
void ServiceLinkFrame(void)
{
    CommBlock  *block;
    LinkPacket *swapPacket;
    void       *swapBuf;

    block = gRam02036338;

    if (block->byte0 != 0) {
        if (block->ready != 0 && block->byte01 != 0 && block->ready06 != 0) {
            block->recvLen = -1;

            swapBuf         = block->bufDPtr;
            block->bufDPtr  = block->bufCPtr;
            block->bufCPtr  = swapBuf;

            swapPacket       = (LinkPacket *)block->bufBPtr;
            block->bufBPtr   = block->packetPtr;
            block->packetPtr = swapPacket;

            block->ready = 0;

            gRam02036338->sendLen  = 0;
            gRam02036338->errorBit =
                (REG_SIOCNT32 << SIO_ERROR_SHIFT) >> 31;

            SIO_PORT.send   = SIO_SEND_IDLE;
            REG_SIOCNT = REG_SIOCNT | SIO_START;
            REG_TM3CNT_H    = TM3_ON_WITH_IRQ;
        }
    } else {
        if (block->retry <= RETRY_LIMIT) {
            block->retry++;
        } else {
            REG_IME = 0;
            *(volatile u16 *)&gBiosIrqFlags =
                *(volatile u16 *)&gBiosIrqFlags | BIOS_IRQ_SERIAL;
            REG_IME = 1;
        }
    }
}
