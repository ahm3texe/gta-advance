/* Fixed-argument calls and DMA flush — 0x08013A28-0x08013A6D
 *
 * The first function makes two calls with fixed arguments. The second disables
 * interrupts and uses DMA3 to copy from the source at gRam02022F60 +4 to
 * gRam02022FA0 (0x84000008 = 32-bit, 8 words), then clears the flag.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/dma_flush.c
 */

#include "gba_io.h"

#define ARG_ID        90
#define ARG_KIND      3
#define ARG_MODE      7
#define DMA_COPY_8W   0x84000008

typedef struct FlushSrc {
    u8    flag;                 /* +0x00 */
    u8    pad01[3];
    void *src;                  /* +0x04 */
} FlushSrc;

extern FlushSrc gRam02022F60;
extern u8       gRam02022FA0[];

extern u32  FUN_08028ec0(u32 a, u32 b, u32 c, u32 d);
extern void FUN_080137a4(u32 a, u32 b, u32 c, u32 d);

/* 0x08013A28 */
void SubmitFixed(void)
{
    FUN_080137a4(0, ARG_ID, ARG_MODE, FUN_08028ec0(ARG_ID, ARG_KIND, 0, 0));
}

/* 0x08013A48 */
void FlushBlock(void)
{
    u16 ime;

    ime = REG_IME;
    REG_IME = 0;

    REG_DMA3.src = gRam02022F60.src;
    REG_DMA3.dst = gRam02022FA0;
    REG_DMA3.control = DMA_COPY_8W;
    REG_DMA3.control;

    REG_IME = ime;
    gRam02022F60.flag = 0;
}
