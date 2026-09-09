/* Bumping the rank counter + the warning flag — 0x08066B40-0x08066C93
 *
 * The twin of bump_rank_counter.c (0x08066C94): the same GetRecordIndex key,
 * the same "increment, saturate at 20" pattern.  The difference is that these
 * counters live inside the single u32 at +0x70 of the record buffer and that
 * there are two threshold tests (19 and 9) per counter -- if either threshold
 * is hit, the gRam02025810[0x137D] warning flag is set to 1.
 *
 * The bit layout of the fields decides which instruction agbcc emits:
 *   bits 8-12  -> a single byte  (ldrb/strb +0x71, mask 0x1F)
 *   bits 13-17 -> a full word    (ldr/str  +0x70, crosses the byte boundary)
 *   bits 18-22 -> a single byte  (ldrb/strb +0x72, mask 0x7C)
 *
 * THE INCREMENT/SATURATION is written as a bitfield; THE THRESHOLD TESTS, on
 * the other hand, are unshifted mask comparisons (see the ROM:
 * "ands r0,#0x7C / cmp r0,#0x4C").  agbcc does not turn a bitfield equality
 * test into a mask -- because the field is promoted to int, fold's
 * optimize_bit_field_compare path is closed and `x.f == 19` always extracts
 * with lsl/lsr.  So the tests are written through the union's raw view
 * (bytes[] / word); since the two see the same address, agbcc shares the base
 * address in a single register too, just as the ROM does.
 *
 * The saturation comparison is UNSIGNED (the ROM has bls).  Because a 5-bit
 * field promotes to int, a plain `> 20` produces a signed `ble`; the constant
 * was written as 20U.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/link_state_step.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define COUNTER_CAP  20
#define WARN_FLAG    gRam02025810[0x137D]

/* The thresholds in raw (shifted) form: 19 and 9 */
#define A_MASK       0x1F
#define A_HIGH       (19 << 0)
#define A_LOW        ( 9 << 0)

#define B_MASK       0x3E000
#define B_HIGH       (19 << 13)
#define B_LOW        ( 9 << 13)

#define C_MASK       0x7C
#define C_HIGH       (19 << 2)
#define C_LOW        ( 9 << 2)

typedef struct StepCounters {
    u8 pad00[0x70];
    union {
        struct {
            u32 spare  : 8;     /* bit 0-7   */
            u32 countA : 5;     /* bit 8-12  */
            u32 countB : 5;     /* bit 13-17 */
            u32 countC : 5;     /* bit 18-22 */
        } f;
        u32 word;
        u8  bytes[4];
    } u;
} StepCounters;

extern StepCounters gSaveBuffer;
extern s32 GetRecordIndex(void);

/* 0x08066B40 */
void BumpStepCounter(void)
{
    switch (GetRecordIndex()) {
    case 0:
        if ((gSaveBuffer.u.bytes[1] & A_MASK) == A_HIGH)
            WARN_FLAG = 1;
        if ((gSaveBuffer.u.bytes[1] & A_MASK) == A_LOW)
            WARN_FLAG = 1;
        gSaveBuffer.u.f.countA++;
        if (gSaveBuffer.u.f.countA > (u32)COUNTER_CAP)
            gSaveBuffer.u.f.countA = COUNTER_CAP;
        break;
    case 1:
        if ((gSaveBuffer.u.word & B_MASK) == B_HIGH)
            WARN_FLAG = 1;
        if ((gSaveBuffer.u.word & B_MASK) == B_LOW)
            WARN_FLAG = 1;
        gSaveBuffer.u.f.countB++;
        if (gSaveBuffer.u.f.countB > (u32)COUNTER_CAP)
            gSaveBuffer.u.f.countB = COUNTER_CAP;
        break;
    case 2:
        if ((gSaveBuffer.u.bytes[2] & C_MASK) == C_HIGH)
            WARN_FLAG = 1;
        if ((gSaveBuffer.u.bytes[2] & C_MASK) == C_LOW)
            WARN_FLAG = 1;
        gSaveBuffer.u.f.countC++;
        if (gSaveBuffer.u.f.countC > (u32)COUNTER_CAP)
            gSaveBuffer.u.f.countC = COUNTER_CAP;
        break;
    }
}
