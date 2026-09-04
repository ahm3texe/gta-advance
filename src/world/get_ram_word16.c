/* RamBlock +0x16 getirici — 0x08062654-0x0806265F
 *
 * RamBlock tanimi src/world/ram_state.c ile BIREBIR AYNI tutulmali;
 * ayni sembole celiskili govde `make check`i kirar (TYPES-001).
 *
 * tools/find_accessors.py ile bulundu.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/get_ram_word16.c
 */

#include "gba_types.h"

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

/* 0x08062654 */
u32 GetRamWord16(void) { return gRam02035EA0.word16; }
