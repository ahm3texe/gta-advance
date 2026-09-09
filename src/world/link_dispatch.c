/* The serial (SIO) interrupt handler — 0x08066904-0x08066A3F
 *
 * Called when a multiplayer transfer finishes.  In order:
 *   - if the block is active (byte0 == 1) it lowers SIOCNT's start bit,
 *   - takes the four SIOMULTI registers (0x04000120) onto the stack in a
 *     single eight-byte copy,
 *   - reads SIOCNT as 32 bits and writes the error bit (bit 6) into errorBit,
 *   - if 0xFEFE (the idle word) came from slot 0 and the receive counter has
 *     completed, it closes the frame: pulls the receive counter to -1, swaps
 *     the receive buffers, and if the send has finished, swaps the send
 *     buffers too, clears the counter and, with IME off, adds the serial bit
 *     to the BIOS interrupt flag,
 *   - if the send counter has not passed 9, it puts the next halfword into
 *     SIOMLT_SEND and advances the counter,
 *   - if the receive counter is not negative, it writes that frame's word into
 *     each of the four slots (24-byte stride); if the counter is 9 it marks
 *     the packet ready, then advances the counter,
 *   - if the block is active it stops Timer3; if the send counter has not
 *     passed 10 it re-enables the start bit and Timer3 together with the
 *     interrupt,
 *   - finally it clears the retry counter.
 *
 * THREE MEASURED DETAILS (each of them decides the match):
 *
 * 1) The SIOMULTI copy MUST be 4-aligned.  A bare `u16 data[4]` produces a
 *    2-aligned structure and agbcc calls `memcpy`; unioned with `u32 word[2]`
 *    it drops to the ROM's `ldr [r0,#4] / ldr [r0,#0] / str / str` pair.
 *
 * 2) The SIOMLT_SEND write wants a NON-VOLATILE struct view.  `REG_SIO.send = x`
 *    from gba_io.h (a volatile SioRegs) makes agbcc emit a redundant
 *    `ldrh [r2,#2]` before the write; a non-volatile view of the same address
 *    leaves only the `strh`.
 *
 * 3) The first SIOCNT read must NOT be volatile while the BIOS flag MUST be --
 *    both measured:
 *      - `cnt = REG_SIOCNT` (volatile) keeps the read in place but produces an
 *        `ldrh r0 / adds r2,r0,#0` pair (+2 instructions, +4 bytes with
 *        alignment).  A non-volatile `SIO_PORT.control` read is hoisted to the
 *        top of the function and gives the ROM's `ldrh r2,[r3]`.
 *      - `gBiosIrqFlags | 0x80` (non-volatile) loads the constant BEFORE the
 *        memory read and redistributes that whole block across other
 *        registers; with `*(vu16 *)&gBiosIrqFlags` the ROM's
 *        `ldrh r0 / movs r1,#128 / orrs r0,r1` order comes back, and so does
 *        the zero constant staying in r4.  That was the only difference: 13
 *        instructions, 0 bytes of size difference.
 *
 * SPELLINGS RULED OUT (all measured, none made a difference):
 *   making cnt u16/u32/s32, moving its declaration up, nested `if`s,
 *   initializing it at its definition; `gBiosIrqFlags |= ...`,
 *   `0x80 | gBiosIrqFlags`, taking it into a local first, narrowing with
 *   `(u16)`.  Adding the constant with `+` and writing IME non-volatile BREAK
 *   the match.
 *
 * NOTE (a different file, not changed here): src/world/link_service.c falls
 * into the same two traps -- `REG_SIO.send` produces an extra `ldrh`, and
 * because `gBiosIrqFlags | 0x80` is not volatile that function stays at
 * 148/152 bytes.  The two spellings above can be applied there too.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/link_dispatch.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "comm_block.h"

#define SIO_START        0x0080
#define SIO_START_CLEAR  0xFF7F
#define SIO_SEND_IDLE    0xFEFE
#define SIO_ERROR_SHIFT  25         /* moves bit 6 to the top of the word */
#define TM3_ON_WITH_IRQ  0x00C0
#define BIOS_IRQ_SERIAL  0x0080
#define SLOT_COUNT       4
#define SLOT_STRIDE      24
#define SEND_LAST        9          /* the index of the last halfword in the buffer */
#define FRAME_DONE       10         /* the counters' upper limit */

/* The four SIOMULTI registers are read as a single block.  The union's u32
 * member raises the alignment to 4; without it agbcc turns the copy into a
 * memcpy. */
typedef union SioMulti {
    u32 word[2];
    u16 data[SLOT_COUNT];
} SioMulti;

#define REG_SIOMULTI (*(volatile SioMulti *)0x04000120)

/* The NON-volatile view of SIOCNT + SIOMLT_SEND: the first control read and
 * the send write use it (see the header comment). */
typedef struct SioPort {
    u16 control;                    /* +0x00 */
    u16 send;                       /* +0x02 */
} SioPort;

#define SIO_PORT  (*(SioPort *)REG_SIOCNT_ADDR)
#define BIOS_IF   (*(vu16 *)&gBiosIrqFlags)

/* 0x08066904 */
void SerialIrqHandler(void)
{
    SioMulti recv;
    void    *swapBuf;
    s32      i;
    u16      cnt;

    cnt = SIO_PORT.control;
    if (gRam02036338->byte0 == 1 && (cnt & SIO_START) != 0)
        REG_SIOCNT = REG_SIOCNT & SIO_START_CLEAR;

    recv = REG_SIOMULTI;

    gRam02036338->errorBit = (REG_SIOCNT32 << SIO_ERROR_SHIFT) >> 31;

    if (recv.data[0] == SIO_SEND_IDLE && gRam02036338->recvLen > SEND_LAST) {
        gRam02036338->recvLen = -1;

        swapBuf               = gRam02036338->bufDPtr;
        gRam02036338->bufDPtr = gRam02036338->bufCPtr;
        gRam02036338->bufCPtr = swapBuf;

        if (gRam02036338->ready != 0) {
            swapBuf                 = gRam02036338->bufBPtr;
            gRam02036338->bufBPtr   = gRam02036338->packetPtr;
            gRam02036338->packetPtr = swapBuf;
            gRam02036338->ready     = 0;
            gRam02036338->sendLen   = 0;
        }

        REG_IME = 0;
        BIOS_IF = BIOS_IF | BIOS_IRQ_SERIAL;
        REG_IME = 1;
    }

    if (gRam02036338->sendLen <= SEND_LAST)
        SIO_PORT.send = *(u16 *)(gRam02036338->sendLen * 2 +
                                 (u8 *)gRam02036338->bufBPtr);

    if (gRam02036338->sendLen <= FRAME_DONE)
        gRam02036338->sendLen = gRam02036338->sendLen + 1;

    if (gRam02036338->recvLen >= 0) {
        for (i = 0; i < SLOT_COUNT; i++) {
            *(u16 *)(gRam02036338->recvLen * 2 +
                     ((u8 *)gRam02036338->bufCPtr + i * SLOT_STRIDE)) =
                recv.data[i];
        }
        if (gRam02036338->recvLen == SEND_LAST)
            gRam02036338->arrived = 1;
    }

    if (gRam02036338->recvLen <= FRAME_DONE)
        gRam02036338->recvLen = gRam02036338->recvLen + 1;

    if (gRam02036338->byte0 != 0)
        REG_TM3CNT_H = 0;

    if (gRam02036338->sendLen <= FRAME_DONE && gRam02036338->byte0 != 0) {
        REG_SIOCNT = REG_SIOCNT | SIO_START;
        REG_TM3CNT_H = TM3_ON_WITH_IRQ;
    }

    gRam02036338->retry = 0;
}
