/* IsRamModeWanted — 0x08062530-0x0806254B
 *
 * gRam02035EA0.unk00!=0 && .mode==8 ise 1, degilse 0.
 *
 * HENUZ ESLESMIYOR: 14 komutun 10'u tutuyor, 4 bayt fark -- dallarin
 * yonu (`return 0` ve `return 1` sirasi) hem duz hem tersi biciminde
 * ayni cikti veriyor. agbcc iki bicimi de aynı yerlesime normalliyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/is_ram_mode.c
 */

#include "gba_types.h"

#define WANTED_MODE    8

typedef struct RamBlock {
    u32 unk00;
    u8  pad04[4];
    u8  type;
    u8  mode;                   /* +0x09 */
} RamBlock;

extern RamBlock gRam02035EA0;

/* 0x08062530 */
u32 IsRamModeWanted(void)
{
    if (gRam02035EA0.unk00 != 0) {
        if (gRam02035EA0.mode == WANTED_MODE)
            return 1;
    }

    return 0;
}
