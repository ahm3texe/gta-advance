/* gRam02035EA0 durum ve ayar — 0x08062294-0x080622BB
 *
 * Iki fonksiyon: mode secici (unk00 null degilse mode==4 ise 2, degilse 1)
 * ve iki alan yazici (+20 u8, +22 u16).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/ram_state.c
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
