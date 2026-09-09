/* IsRamModeWanted — 0x08062530-0x0806254B
 *
 * Return 1 if gRam02035EA0.unk00 != 0 && .mode == 8, otherwise 0.
 *
 * BYTE-MATCHING. Expressing the first condition as an explicit early return 0
 * rather than nesting places the shared zero block before the literal pool.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/is_ram_mode.c
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
    u8  pad18[4];
    u16 word1C;                 /* +0x1C */
    u16 word1E;                 /* +0x1E */
    u8  pad20[4];               /* out to the 36 bytes 0x08061D34 clears */
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
