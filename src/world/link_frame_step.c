/* The link frame step — 0x0806660C-0x0806673B
 *
 * Called once per frame.  The SIO control word is taken first in a single
 * 32-bit read, then a three-state machine is run on the communication block's
 * +0x01 state byte (the states FALL THROUGH into each other, there are no
 * breaks):
 *
 *   state 0 — Setup.  It only proceeds if this GBA is the master (the ID
 *             field, bits 4-5, is zero).  If SD (bit 3) is set and busy
 *             (bit 7) is not, and additionally SI (bit 2) is clear and the
 *             send length is 12, the hardware is switched from the serial
 *             interrupt to the timer-3 interrupt: IME off; serial (bit 7) is
 *             disabled and timer-3 (bit 6) enabled in IE; IME on; SIOCNT's
 *             interrupt bit (bit 14) is cleared; TM3CNT gets a 0xABFB reload +
 *             a disabled control in a single 32-bit write; 0xC0 is written to
 *             IF to acknowledge the two pending interrupts; 8 is written to
 *             the block's +0x00 byte.  Whether the condition holds or not, it
 *             moves to state 1; the whole machine is skipped only when the
 *             SD/busy condition does not hold.
 *   state 1 — The wait counter.  If the +0x02 flag is set, the counter at
 *             +0x08 is incremented up to 8, and on saturation it moves to
 *             state 2.
 *   state 2 — ReceiveLinkPackets is called.
 *
 * Then the +0x0B frame counter is incremented and a single-word status summary
 * is returned to the caller: the low bits of +0x03, +0x02 << 8, 0x80 if
 * +0x00 == 8, 0x1000 for the error flag, and 0x8000 if the wait counter has
 * saturated.  The 0x2000 branch is UNREACHABLE in the ROM (a two-bit ID field
 * can never exceed 3), but the ROM emits those instructions, so they are in
 * the source too.
 *
 * MEASURED SPELLING RULES (each tried individually; changing them breaks the
 * match):
 *
 *  1. SIOCNT is both read as 32 bits and has a bit cleared in its +1 byte; the
 *     ROM keeps a SINGLE base register for both, so a union is required.
 *     Writing bit 14 as a FIELD is mandatory: a bitfield assignment makes
 *     agbcc produce the mask in 32 bits (`movs #65 / negs`), while a plain
 *     `&= ~0x40` gives `movs #0xBF` (2 bytes short).
 *  2. The mask results are taken into u8 locals.  With a u32 local agbcc
 *     merges the constant and the result into the same pseudo register and
 *     produces `movs r0,#0x30 / ands r0,r6`; with a u8 local the constant is
 *     born in QImode, regmove's merge is prevented, and the ROM's
 *     `movs r1,#0x30 / adds r0,r6,#0 / ands r0,r1` sequence comes out.  No
 *     extra truncation instruction appears: the masks are already in the low
 *     byte and agbcc eliminates the narrowing.
 *  3. The `status = tmp;` intermediate assignment preserves the ROM's copy at
 *     the merge point (`adds r3,r0,#0`); with a direct assignment agbcc merges
 *     the two branches into a shared final instruction and loses 2 bytes.
 *  4. `lo` first, `hi` second: the ROM reads +0x03 before +0x02.  The OR order
 *     in the branches was measured too (`0x80 | lo | hi` and `lo | hi`).
 *  5. TM3CNT and IF are written through separate macros; cse derives the
 *     second constant address as +0xF6 from the first one's register and gives
 *     the ROM.  Hand-written pointer arithmetic produces the same instructions
 *     but the opposite registers.
 *  6. The block's three regions are held in THREE SEPARATE locals; a single
 *     variable inflates the reference count and shifts the allocator's order
 *     (an r4/r5 swap).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/link_frame_step.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "comm_block.h"

/* SIOCNT: a single 32-bit read plus a field view for bit 14 (the serial
 * interrupt enable); both must go through the same base register (rule 1). */
typedef union SioCtrl {
    u32 word;
    struct {
        u32 low14 : 14;             /* bit 0-13 */
        u32 irq   : 1;              /* bit 14 — the serial interrupt enable */
        u32 high  : 17;             /* bit 15-31 */
    } bits;
} SioCtrl;

#define SIOC (*(volatile SioCtrl *)REG_SIOCNT_ADDR)

#define SIO_ID_MASK      0x30       /* bits 4-5: the multiplayer id */
#define SIO_SD_BUSY      0x88       /* bit 3 SD, bit 7 busy */
#define SIO_SD_READY     0x08       /* SD set, not busy */
#define SIO_SI           0x04       /* bit 2 */

#define IE_CLEAR_SERIAL  0xff7f     /* the mask that clears bit 7 of IE */
#define IE_TIMER3        0x40
#define IF_TIMER3_SERIAL 0xc0
#define TM3_RELOAD_OFF   0xabfb     /* the low half is the reload,
                                       the high half is control = disabled */
#define SEND_LEN         12
#define WAIT_CAP         7

#define ST_IDENT_READY   0x0080
#define ST_ERROR         0x1000
#define ST_ID_HIGH       0x2000

extern u32 ReceiveLinkPackets(u8 *dest);

/* 0x0806660C */
u32 StepLinkFrame(u8 *dest)
{
    CommBlock *setup;
    CommBlock *wait;
    CommBlock *tail;
    u32 cnt;
    u8  id;
    u8  sd;
    u8  si;
    u32 status;
    u32 tmp;
    u32 capped;
    u32 lo;
    u32 hi;

    cnt = SIOC.word;
    setup = gRam02036338;

    switch (setup->byte01) {
    case 0:
        id = cnt & SIO_ID_MASK;
        if (id == 0) {
            sd = cnt & SIO_SD_BUSY;
            if (sd != SIO_SD_READY)
                break;
            si = cnt & SIO_SI;
            if (si == 0 && setup->sendLen == SEND_LEN) {
                REG_IME = 0;
                REG_IE &= IE_CLEAR_SERIAL;
                REG_IE |= IE_TIMER3;
                REG_IME = 1;
                SIOC.bits.irq = 0;
                REG_TM3CNT = TM3_RELOAD_OFF;
                REG_IF = IF_TIMER3_SERIAL;
                setup->byte0 = sd;          /* sd is SIO_SD_READY here */
            }
        }
        gRam02036338->byte01 = 1;
        /* fall through */
    case 1:
        wait = gRam02036338;
        if (wait->byte02 != 0) {
            if (wait->pad08 <= WAIT_CAP)
                wait->pad08++;
            else
                wait->byte01 = 2;
        }
        /* fall through */
    case 2:
        ReceiveLinkPackets(dest);
        break;
    }

    gRam02036338->ident++;

    tail = gRam02036338;
    lo = tail->byte03;
    hi = tail->byte02 << 8;
    if (tail->byte0 == SIO_SD_READY)
        tmp = ST_IDENT_READY | lo | hi;
    else
        tmp = lo | hi;
    status = tmp;

    if (gRam02036338->errorBit != 0)
        status |= ST_ERROR;

    capped = (gRam02036338->pad08 >> 3) << 15;

    /* A two-bit ID field cannot exceed 3; the branch is dead in the ROM too,
       but it is emitted. */
    return (((cnt << 26) >> 30) > 3) ? (ST_ID_HIGH | status | capped)
                                     : (status | capped);
}
