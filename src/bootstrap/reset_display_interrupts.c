/* Ekran ve kesme sifirlama — 0x080007B4-0x0800082B
 *
 * VRAM ve OAM'i DMA3 ile temizler, VBlank bekler ve kesme tablosunu
 * yeniden kurar.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/bootstrap/reset_display_interrupts.c
 */

#include "gba_io.h"

#define DMA_CLEAR_VRAM 0x8100C000   /* enable | fixed source | 0xC000 halfword */
#define DMA_CLEAR_OAM  0x81000200   /* enable | fixed source | 0x200 halfword  */

#define SUBSYSTEM_ARGUMENT 0x2FD

extern volatile u8 gVBlankState;
extern u32 gDisplayState;
/* BIOS kesme denetim bayraklari (IntrWait). volatile DEGIL: volatile
 * isaretlenince agbcc yukleme sirasini degistiriyor ve ROM'dan sapiyor. */

/* Asagidaki uc fonksiyon henuz adlandirilmadi. Assembly kaynaginda
 * WaitForDma3 / InitSubsystem / WaitForVBlank diye etiketlenmislerdi ama
 * disassembly bu isimleri desteklemiyor: FUN_08063b74 bir DMA dongusu degil,
 * dort donanim register'ina sabit yaziyor; FUN_0800cae4 VBlank beklemiyor,
 * iki fonksiyon cagiriyor. Dogrulanana kadar Ghidra adlari kullaniliyor. */
extern void FUN_08063b74(void);
extern void FUN_0803251c(u32 argument);
extern void FUN_0800cae4(void);
extern void InitInterrupts(void);

/* 0x080007B4 */
void ResetDisplayAndInterrupts(void)
{
    volatile u16 fill;

    FUN_08063b74();

    fill = 0;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = VRAM_BASE;
    REG_DMA3.control = DMA_CLEAR_VRAM;
    REG_DMA3.control;

    fill = 0;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = OAM_BASE;
    REG_DMA3.control = DMA_CLEAR_OAM;
    REG_DMA3.control;

    gVBlankState = 1;
    gDisplayState = 0;

    FUN_0803251c(SUBSYSTEM_ARGUMENT);
    FUN_0800cae4();

    gBiosIrqFlags |= 1;
    InitInterrupts();
}
