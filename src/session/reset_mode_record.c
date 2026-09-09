/* Clear the mode record, then the stat table — 0x08061DD4-0x08061DEB
 *
 * The 180-byte clear is what fixes the record's size in data/ram_map.csv, which
 * had only the mode byte before.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/session/reset_mode_record.c
 */

#include "gba_types.h"

#define MODE_RECORD_SIZE  180

extern u8 gRam02036050[];

extern void *Memset(void *dest, int value, u32 count);
extern void  FUN_080621b8(void);

/* 0x08061DD4 */
void FUN_08061dd4(void)
{
    Memset(gRam02036050, 0, MODE_RECORD_SIZE);
    FUN_080621b8();
}
