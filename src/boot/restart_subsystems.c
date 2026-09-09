/* Restart the six subsystems — 0x08063DAC-0x08063DCB
 *
 * Only the first call takes an argument; the rest take none, and the ROM leaves
 * r0 alone between them, so each is declared void.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/boot/restart_subsystems.c
 */

#include "gba_types.h"

extern void FUN_08062c24(u32 mode);
extern void FUN_08063bd8(void);
extern void FUN_080333ac(void);
extern void FUN_0802f55c(void);
extern void FUN_0803493c(void);
extern void FUN_0800ab6c(void);

/* 0x08063DAC */
void FUN_08063dac(void)
{
    FUN_08062c24(1);
    FUN_08063bd8();
    FUN_080333ac();
    FUN_0802f55c();
    FUN_0803493c();
    FUN_0800ab6c();
}
