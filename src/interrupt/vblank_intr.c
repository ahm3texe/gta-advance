/* VBlank interrupt handler — 0x08000220-0x0800038C
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/interrupt/vblank_intr.c
 *
 * On entering VBlank two frame counters are incremented, then the nesting
 * counter (gEepromAvailable) is incremented by one. If the counter is not 1
 * the handler is already running, and it returns without doing any work. The
 * real work is decided by VCOUNT: depending on how much of the VBlank window
 * is left, either all of the transfer steps or none of them are run.
 * gVBlankState is the small state machine that carries that decision across
 * frames.
 */

#include "gba_io.h"

/* The IWRAM address is written as a constant cast, not as an extern symbol:
 * the ROM produces 0x03000000 by shifting (movs #0xc0 / lsls #18); as a
 * symbol it would be read from the literal pool. (docs/COMPILER.md, the
 * exception to rule 1.) */
#define gFrameDelay (*(u32 *)0x03000000)

/* VBlank starts at scanline 160; VCOUNT - 160 is the number of lines elapsed. */
#define VBLANK_FIRST_LINE 160
/* Short budget: the delay still allowed for a transfer when the state machine
 * is already ready. Long budget: the delay accepted when starting for the
 * first time. */
#define BUDGET_SHORT 10
#define BUDGET_LONG  19

#define FRAME_DELAY_MAX 5

/* gVBlankState degerleri */
#define VBLANK_IDLE  0   /* a transfer will be attempted next VBlank */
#define VBLANK_BUSY  1   /* the transfer was done this frame         */
#define VBLANK_READY 2   /* the window was missed; transfer directly next frame */

extern u32 gFrameCounterLate;
extern u32 gIwramFrameCounter;
/* The same word is used both as the EEPROM-found flag and as the interrupt
 * nesting counter (data/ram_map.csv 0x02000EB8). */
extern u32 gEepromAvailable;
extern u16 gVBlankEnabled;
/* volatile is REQUIRED: without it agbcc loads the value once for the three
 * comparisons and keeps it in a callee-saved register; the ROM re-reads it in
 * every branch (0x8000270, 0x80002E0, 0x8000358). Here volatile is not a
 * semantic marker but an ordering/reload switch — docs/COMPILER.md rule 4. */
extern volatile u8 gVBlankState;
extern u32 gAsyncState;
extern u8  gGameState[16];

/* The subsystems that run during VBlank; not named yet. */
extern void FUN_08033264(void);
extern void FUN_08011ec4(void);
extern void ServiceLinkFrame(void);
extern void FlushSpriteList(void);
extern void FUN_080133a8(void);
extern void FUN_080130f4(void);
extern void FlushPaletteQueue(void);
extern void FUN_080101d8(void);
extern void FUN_080108f4(void);
extern void NoOpVBlankFinalize(void);

/* 0x08000220 */
void VBlankIntr(void)
{
    u32 depth;
    u32 counter;
    u8 phase;

    gFrameCounterLate++;
    gIwramFrameCounter++;

    /* A nested entry does not repeat the work, it only carries the counter. */
    depth = ++gEepromAvailable;
    if (depth == 1) {
        FUN_08033264();
        FUN_08011ec4();
        if (gVBlankEnabled != 0)
            ServiceLinkFrame();

        if ((u16)(REG_VCOUNT - VBLANK_FIRST_LINE) <= BUDGET_SHORT) {
            if (gVBlankState == VBLANK_READY) {
                /* The window was missed last frame: transfer directly. */
                FlushSpriteList();
                FUN_080133a8();
                FUN_080130f4();
                FlushPaletteQueue();
                if (gAsyncState == 0)
                    FUN_080101d8();

                gFrameDelay = counter = gIwramFrameCounter;
                if (counter > FRAME_DELAY_MAX)
                    gFrameDelay = FRAME_DELAY_MAX;
                phase = gGameState[12] - 1;
                if (phase <= 1)
                    gFrameDelay = FRAME_DELAY_MAX;

                gIwramFrameCounter = 0;
                gVBlankState = VBLANK_BUSY;
            } else if (gVBlankState == VBLANK_IDLE) {
                if (gAsyncState == 0)
                    FUN_080108f4();

                /* The step above may have eaten time; VCOUNT is re-read.
                 * The condition is written inverted, as in the ROM: that makes
                 * agbcc place the "then" branch first (cmp #19 / bls). */
                if ((u16)(REG_VCOUNT - VBLANK_FIRST_LINE) > BUDGET_LONG) {
                    gVBlankState = VBLANK_READY;
                } else {
                    FlushSpriteList();
                    FUN_080133a8();
                    FUN_080130f4();
                    FlushPaletteQueue();
                    if (gAsyncState == 0)
                        FUN_080101d8();

                    gFrameDelay = counter = gIwramFrameCounter;
                    if (counter > FRAME_DELAY_MAX)
                        gFrameDelay = FRAME_DELAY_MAX;
                    phase = gGameState[12] - 1;
                    if (phase <= 1)
                        gFrameDelay = FRAME_DELAY_MAX;

                    gIwramFrameCounter = 0;
                    gVBlankState = VBLANK_BUSY;
                }
            } else if (gVBlankState != VBLANK_BUSY) {
                gVBlankState = VBLANK_IDLE;
            }
        }

        NoOpVBlankFinalize();
    }

    gEepromAvailable--;
    gBiosIrqFlags |= 1;
}

/* Notes (tried and measured):
 *  - The transfer block is written out twice. Factored into a shared helper,
 *    agbcc -O2 does not inline it and emits two `bl`s; in the ROM the code is
 *    duplicated (the same reasoning as docs/COMPILER.md rule 14).
 *  - Without volatile on gVBlankState: 356 bytes, 168 bytes differing. The
 *    compiler keeps the result of the first `ldrb` in r4 and reuses it for the
 *    second and third comparisons, while the address goes to r7; the ROM does
 *    exactly the opposite (address in r4, value re-read in every branch).
 *  - Written plainly (`<= BUDGET_LONG`), the second VCOUNT condition makes
 *    agbcc emit `bhi else`; the ROM uses `bls then`, so the condition was
 *    written inverted in the source. That also changes where the second
 *    literal pool sits (in the ROM the pool is between `movs #2 / b` and the
 *    transfer block).
 *  - gFrameCounterLate / gEepromAvailable / gVBlankEnabled / gAsyncState /
 *    gGameState / gBiosIrqFlags were left as extern symbols: the ROM reads all
 *    of them from the literal pool. gFrameDelay is the opposite -- it is
 *    produced by shifting.
 */
