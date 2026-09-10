/* Queue the argument offset by a base halfword — 0x0800629C-0x080062B7
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/queue_offset_by_base.c
 */

#include "gba_types.h"

extern u16 gRam02010C50;

extern void FUN_080046dc(u32 a, u32 b, u32 c);

/* 0x0800629C */
void FUN_0800629c(u32 value)
{
    FUN_080046dc(3, gRam02010C50 + value, 0);
}
