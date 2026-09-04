/* IsRamModeWanted — 0x08062530-0x0806254B
 *
 * gRam02035EA0.unk00!=0 && .mode==8 ise 1, degilse 0.
 *
 * BYTE-MATCHING. Ilk kosulu ic ice blok yerine acik erken `return 0`
 * yapmak, ortak sifir blogunu literal havuzundan once yerlestiriyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/is_ram_mode.c
 */

#include "gba_types.h"

#define WANTED_MODE    8

typedef struct RamBlock {
    u32 unk00;                  /* +0x00 */
    u8  pad04[4];
    u8  type;                   /* +0x08 */
    u8  mode;                   /* +0x09 */
    u8  pad0A[10];
    u8  byte14;                 /* +0x14 */
    u8  pad15[1];
    u16 word16;                 /* +0x16 */
} RamBlock;

extern RamBlock gRam02035EA0;

/* 0x08062530 */
u32 IsRamModeWanted(void)
{
    if (gRam02035EA0.unk00 == 0)
        return 0;

    if (gRam02035EA0.mode == WANTED_MODE)
        return 1;

    return 0;
}
