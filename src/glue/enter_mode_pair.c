/* Enter mode 0 and mode 1 — 0x0800AB60-0x0800AB6B and 0x0800AB6C-0x0800AB77
 *
 * Two adjacent wrappers over FUN_080657D8, differing only in the constant.
 * src/boot/restart_subsystems.c calls the second of them.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/enter_mode_pair.c
 */

#include "gba_types.h"

extern void FUN_080657d8(s32 code);

/* 0x0800AB60 */
void FUN_0800ab60(void)
{
    FUN_080657d8(0);
}

/* 0x0800AB6C */
void FUN_0800ab6c(void)
{
    FUN_080657d8(1);
}
