/* IRQ yardimcilari — 0x08000730-0x080007B3
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/interrupt/irq_helpers.c
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

/* volatile SART: kaldirilinca agbcc sabiti bellekten once yukluyor ve
 * ROM'dan sapiyor. Ayni erisim bicimi gBiosIrqFlags'te tam tersini
 * gerektiriyordu; volatile burada semantik degil siralama dugmesi. */
#define REG_IF          (*(volatile u16 *)0x04000202)
#define FRAME_DELAY_MAX 5

extern u8  gVBlankState;
extern u32 gAsyncState;
extern u8  gGameState[16];
/* IWRAM adresleri sabit cast olarak yazilir, extern sembol olarak degil:
 * ROM 0x03000000'i kaydirmayla uretiyor (movs #0xc0 / lsls #18), sembol
 * olsaydi literal havuzdan okunurdu. */
#define gFrameDelay        (*(u32 *)0x03000000)
#define gIwramFrameCounter (*(u32 *)0x03000004)

/* VBlank sirasinda calisan aktarim adimlari; henuz adlandirilmadi. */
extern void FUN_08012b9c(void);
extern void FUN_080133a8(void);
extern void FUN_080130f4(void);
extern void FUN_08013900(void);
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

    FUN_08012b9c();
    FUN_080133a8();
    FUN_080130f4();
    FUN_08013900();

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
