/* Copy a block into IWRAM, then clear three flags — 0x0800CAE4-0x0800CAFB
 *
 * 30464 is `movs r2,#238 / lsls r2,#7`, and the IWRAM destination 0x03000100
 * comes through the literal pool.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/vram/copy_to_iwram_then_clear.c
 */

#include "gba_types.h"

#define IWRAM_DEST  ((void *)0x03000100)
#define COPY_SIZE   (238 << 7)      /* 30464 */
#define COPY_MODE   128

extern u8 gBufferBase02014ED0[];

extern void FUN_0800c690(void *src, void *dest, u32 size, u32 mode);
extern void ZeroThreeFlags(void);

/* 0x0800CAE4 */
void FUN_0800cae4(void)
{
    FUN_0800c690(gBufferBase02014ED0, IWRAM_DEST, COPY_SIZE, COPY_MODE);
    ZeroThreeFlags();
}
