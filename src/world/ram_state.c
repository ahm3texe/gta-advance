/* gRam02035EA0 state and settings — 0x08062294-0x080622BB
 *
 * Two functions: a mode selector (when unk00 is non-null, return 2 for
 * mode == 4, otherwise 1), and a setter for +20 u8 and +22 u16.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/ram_state.c
 */

#include "gba_types.h"

#define MODE_SPECIAL   4

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

/* 0x08062294 */
u32 GetModeCategory(void)
{
    if (gRam02035EA0.unk00 == 0)
        return 0;
    if (gRam02035EA0.mode == MODE_SPECIAL)
        return 2;

    return 1;
}

/* 0x080622B4 */
void SetRamState(u8 b, u16 w)
{
    gRam02035EA0.byte14 = b;
    gRam02035EA0.word16 = w;
}
