/* Release the block at 0x02026DA0 — 0x080313E0-0x080313EF
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/release_block_da0.c
 */

#include "gba_types.h"

extern u8 gRam02026DA0[];

extern void ReleaseObject(u32 *block);

/* 0x080313E0 */
void FUN_080313e0(void)
{
    ReleaseObject((u32 *)gRam02026DA0);
}
