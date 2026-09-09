/* Write window settings — 0x0801D7DC-0x0801D801
 *
 * Two functions. The first sets four fields in 0x020230D0: +0/+4 to
 * 0x01000000 and +12/+16 to (arg<<16 | 0xFF000000). These were interpreted
 * as custom 24.8 fixed-point values resembling BGxHOFS/BGxVOFS settings.
 * The second writes a single pointer.
 *
 * ROM constants: movs #128; lsls #17 = 0x01000000;
 * movs #255; lsls #24 = 0xFF000000.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/window_config.c
 */

#include "gba_types.h"
#include "map_grid.h"

#define UNIT_ONE      0x01000000
#define OFFSET_HIGH   0xFF000000

typedef struct WinConfig {
    u32 fixed0;                 /* +0x00 */
    u32 fixed4;                 /* +0x04 */
    u8  pad08[4];
    u32 valueA;                 /* +0x0C */
    u32 valueB;                 /* +0x10 */
} WinConfig;

extern WinConfig gRam020230D0;

/* 0x0801D7DC */
void SetWindow(u16 a, u16 b)
{
    gRam020230D0.fixed0 = UNIT_ONE;
    gRam020230D0.fixed4 = UNIT_ONE;
    gRam020230D0.valueA = (a << 16) + OFFSET_HIGH;
    gRam020230D0.valueB = (b << 16) + OFFSET_HIGH;
}

/* 0x0801D7FC */
void StoreRam0201AEE8(void *ptr)
{
    gRam0201AEE8 = ptr;
}
