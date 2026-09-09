/* IRQ yardimcilari — 0x08000730-0x080007B3
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/interrupt/irq_helpers.c
 */

#include "gba_io.h"

/* volatile is REQUIRED: without it agbcc loads the constant before the memory
 * access and diverges from the ROM. The same access pattern required exactly
 * the opposite in gBiosIrqFlags; here volatile is not a semantic marker but an
 * ordering switch. */
#define FRAME_DELAY_MAX 5

extern volatile u8 gVBlankState;
extern u32 gAsyncState;
extern u8  gGameState[16];
/* The IWRAM addresses are written as constant casts, not as extern symbols:
 * the ROM produces 0x03000000 by shifting (movs #0xc0 / lsls #18); as symbols
 * they would be read from the literal pool. */
#define gFrameDelay        (*(u32 *)0x03000000)
#define gIwramFrameCounter (*(u32 *)0x03000004)

/* The transfer steps that run during VBlank; not named yet. */
extern void FlushSpriteList(void);
extern void FUN_080133a8(void);
extern void FUN_080130f4(void);
extern void FlushPaletteQueue(void);
extern void FUN_080101d8(void);
extern void FUN_080327c8(void);

/* 0x08000730 */
void NoOpVBlankFinalize(void)
{
}

/* 0x08000734 */
void DummyIntr(void)
{
}

/* 0x08000738 */
void RunVBlankTransfers(void)
{
    u32 counter;
    u8 phase;

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
    gVBlankState = 1;
}

/* 0x08000798 */
void NoOpInterruptHelper(void)
{
}

/* 0x0800079C */
void VCountIntr(void)
{
    FUN_080327c8();
    REG_IF |= 4;
}
