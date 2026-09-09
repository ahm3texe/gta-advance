/* Stopping the audio DMA when the cart flag is set — 0x080337A8-0x0803381F
 *
 * gCartFlag's value is first copied into gRam02027314; if it is not 1 nothing
 * happens.  If it is 1 the flag is cleared and DMA1 and DMA2 (the audio FIFO
 * channels on the GBA) are shut down in two steps: first the control bits with
 * 0xC5FF, then the enable bit with 0x7FFF; the ROM has a read-back after each
 * write.  Finally the buffer at gRam02027310 is released.
 *
 * THREE DETAILS WERE MEASURED:
 *
 * 1) The condition looks at gRam02027314 ITSELF, not at a LOCAL u8.  With a
 *    local, the ROM's `lsls #24 / lsrs #24` zero-extension does not appear
 *    (the ldrb is already extended).
 *
 * 2) The control register must be written through a `vu16 *` BASE + INDEX.
 *    The `REG_DMA1.control` struct view from gba_io.h produces an extra
 *    volatile READ before every write (two extra ldrh per channel).  An
 *    absolute macro (`((vu16 *)0x040000BC)[5]`), on the other hand, folds the
 *    +10 offset into the address constant and puts 0x040000C6 in the pool; in
 *    the ROM the base is 0x040000BC and the offset is 10.
 *
 * 3) The SECOND base must be assigned at its own point of use.  Assigned both
 *    at the top, the two pool loads gather at the top; assigned twice to a
 *    single variable, agbcc pulls the second out as a common subexpression of
 *    the first plus 12 (`adds r4,#12`).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/stop_audio_dma.c
 */

#include "gba_types.h"

#define DMA1_BASE       0x040000BC
#define DMA2_BASE       0x040000C8
#define DMA_CNT_H       5           /* +0x0A, in u16 steps */
#define DMA_CTRL_MASK   0xC5FF      /* ~0x3A00 */
#define DMA_ENABLE_MASK 0x7FFF      /* ~0x8000 */

extern u8   gCartFlag;
extern u8   gRam02027314;
extern u32  gRam02027310;   /* the same view as in zero_three_flags.c: a COUNT, not a pointer */
extern u8   gBufferBase02014ED0[];

extern void FUN_0800c804(u32 *dest, u32 value);

/* 0x080337A8 */
void StopAudioDmaOnCartFlag(void)
{
    vu16 *p1;
    vu16 *p2;

    gRam02027314 = gCartFlag;
    if (gRam02027314 != 1)
        return;

    gCartFlag = 0;

    p1 = (vu16 *)DMA1_BASE;
    p1[DMA_CNT_H] = DMA_CTRL_MASK & p1[DMA_CNT_H];
    p1[DMA_CNT_H] = DMA_ENABLE_MASK & p1[DMA_CNT_H];
    p1[DMA_CNT_H];

    p2 = (vu16 *)DMA2_BASE;
    p2[DMA_CNT_H] = DMA_CTRL_MASK & p2[DMA_CNT_H];
    p2[DMA_CNT_H] = DMA_ENABLE_MASK & p2[DMA_CNT_H];
    p2[DMA_CNT_H];

    if (gRam02027310 != 0)
        FUN_0800c804((u32 *)gBufferBase02014ED0, gRam02027310);
    gRam02027310 = 0;
}
