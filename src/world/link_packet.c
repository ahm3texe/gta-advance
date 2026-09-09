/* Builds the packet to be sent — 0x0806673C-0x08066797
 *
 * Writes the id byte and the XOR of two fields into the header, clears the
 * checksum field, copies the 16-byte payload with CpuSet, then sums the
 * packet's ten half words, writes the checksum as `~total - 12` and raises the
 * "ready" flag.
 *
 * The checksum field is INCLUDED in the sum: since it is cleared beforehand,
 * the result is unaffected.
 *
 * The global is re-read on every access because the writes go through the
 * packet's pointer and may alias the pointer itself; that is why agbcc does
 * not apply CSE.
 *
 * STATUS: PARKED — 92/92 in size, 20 bytes off, a one-instruction divergence.
 *
 * The one remaining difference: after the loop the ROM KEEPS the
 * `gRam02036338` address in r4 (`ldr r2, [r4, #0]`), while our build reloads
 * it from the pool (`ldr r0, [pc]; ldr r2, [r0, #0]`).
 *
 * Measured (dump_alloc --function BuildLinkPacket): agbcc produces TWO
 * separate pseudos for the same address constant --
 *     27  mem[.LC0]  refs 5  live 46  priority 0.217  -> r4
 *     74  mem[.LC0]  refs 2  live  4  priority 0.500  -> r0
 * In the ROM there is a single live range.  The second pseudo is born because
 * CSE does not cross the loop block; this is the opposite direction of rule
 * 50's documented limit, and there is NO MERGING lever on the source side.
 *
 * WHY THERE IS NO SOURCE-SIDE LEVER (re-measured): the two pseudos are born at
 * a CSE basic-block boundary.  p27 is in L0 (before the loop) and p74 in L2
 * (after it); the loop block L1 sits between them and, because it has a back
 * edge, CSE's extended basic block path is cut there.  The constant pool load
 * is re-emitted at every reference, and there is no source expression that
 * would let L2 use L0's value: L2's only predecessor is L1, and L1 has two
 * predecessors.  The one known lever is to hold the address in a local
 * variable (`CommBlock **slot = &gRam02036338;`) -- the same class was
 * rejected in input_extra.c (`irq = &gBiosIrqFlags`) as an invented variable,
 * and it is REJECTED here too.
 *
 * Rules 54-61 were reviewed: none of them applies to this function (no
 * bitfields, no u8 fields, no volatile, no range guard).
 *
 * SPELLINGS RULED OUT (all stayed at the same 20 bytes):
 *   - the counter as s32/u32, do-while/while/for, indexed access  (5 spellings)
 *   - taking the tail block into a local `CommBlock *`
 *   - swapping the order of the two tail assignments (got worse, 36 bytes)
 *   - making `total` u32
 *   - taking the packet pointer into a local BEFORE the loop and using it in
 *     the tail (`LinkPacket *pkt`): got worse, 32 bytes -- in the tail the ROM
 *     RE-READS the packet with `ldr r1,[r2,#28]`.
 * Making the counter u32 took it from 29 to 20 bytes; nothing else changed
 * anything.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/link_packet.c
 */

#include "gba_types.h"
#include "comm_block.h"

#define PACKET_HALFWORDS  10
#define CHECKSUM_BIAS     12
#define CPUSET_COPY_16    0x04000004   /* 32-bit, four words */

extern void CpuSet(const void *src, void *dst, u32 control);

/* 0x0806673C */
void BuildLinkPacket(const void *payload)
{
    u32  i;
    s32  total;
    u16 *word;

    total = 0;

    gRam02036338->packetPtr->ident    = gRam02036338->ident;
    gRam02036338->packetPtr->mix      = gRam02036338->byte02 ^ gRam02036338->byte03;
    gRam02036338->packetPtr->checksum = 0;

    CpuSet(payload, gRam02036338->packetPtr->payload, CPUSET_COPY_16);

    i = 0;
    word = (u16 *)gRam02036338->packetPtr;
    do {
        total += *word;
        word++;
        i++;
    } while (i <= PACKET_HALFWORDS - 1);

    gRam02036338->packetPtr->checksum = ~total - CHECKSUM_BIAS;
    gRam02036338->ready = 1;
}
