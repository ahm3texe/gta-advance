/* Script command: reset when FUN_08061F54 says so — 0x0805AB7C-0x0805AB8F
 *
 * Rule 35: `pop {r0}; bx r0` overwrites r0, so this handler returns void. It is
 * the only one in the block that does; every other one answers 1.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_maybe_reset.c
 */

#include "gba_types.h"

extern u32  FUN_08061f54(void);
extern void FUN_08059818(u32 value);

/* 0x0805AB7C */
void FUN_0805ab7c(void)
{
    if (FUN_08061f54() != 0)
        FUN_08059818(0);
}
