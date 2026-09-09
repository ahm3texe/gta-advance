/* Queue an event with a zero third argument — 0x08063B2C-0x08063B37
 *
 * The first two arguments are forwarded untouched; only the third is supplied.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/boot/queue_event_zero.c
 */

#include "gba_types.h"

extern void FUN_08062dd8(u32 id, u32 a, u32 b);

/* 0x08063B2C */
void FUN_08063b2c(u32 id, u32 a)
{
    FUN_08062dd8(id, a, 0);
}
