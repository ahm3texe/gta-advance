/* gRam02035EA0.type sorgusu — 0x08062514-0x0806252F
 *
 * Kardesi IsRamModeWanted ayri dosyada ve byte-matching.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/ram_flags.c
 */

#include "gba_types.h"

#define SKIP_TYPE     17

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

/* 0x08062514 */
u32 GetRamType(void)
{
    if (gRam02035EA0.unk00 == 0)
        return 0;
    if (gRam02035EA0.type == SKIP_TYPE)
        return 0;

    return gRam02035EA0.type;
}
