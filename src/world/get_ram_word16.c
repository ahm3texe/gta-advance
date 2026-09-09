/* RamBlock +0x16 getter — 0x08062654-0x0806265F
 *
 * Keep RamBlock IDENTICAL to src/world/ram_state.c; conflicting definitions
 * for the same symbol fail make check (TYPES-001).
 *
 * Found with tools/find_accessors.py.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/get_ram_word16.c
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
    u8  pad18[4];
    u16 word1C;                 /* +0x1C */
    u16 word1E;                 /* +0x1E */
    u8  pad20[4];               /* out to the 36 bytes 0x08061D34 clears */
} RamBlock;

extern RamBlock gRam02035EA0;

/* 0x08062654 */
u32 GetRamWord16(void) { return gRam02035EA0.word16; }
