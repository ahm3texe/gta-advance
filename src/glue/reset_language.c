/* Reset the language and its companion word — 0x0805E14C-0x0805E163
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/reset_language.c
 */

#include "gba_types.h"

extern u32 gRam02035C8C;

extern void SetLanguage(int index);

/* 0x0805E14C */
void FUN_0805e14c(void)
{
    SetLanguage(0);
    gRam02035C8C = 0;
}
