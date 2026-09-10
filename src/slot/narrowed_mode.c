/* The callee's answer, narrowed to 16 bits — 0x0804FB3C-0x0804FB49
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/slot/narrowed_mode.c
 */

#include "gba_types.h"

extern u32 FUN_0804eeac(void);

/* 0x0804FB3C */
u16 FUN_0804fb3c(void)
{
    return FUN_0804eeac();
}
