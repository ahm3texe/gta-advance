/* Forwarding wrapper — 0x0803C044-0x0803C04D
 *
 * The body only calls FUN_08055700, whose arguments are already in r0-r3 and
 * are not touched. Rule 35: `pop {r1}; bx r1` means r0 carries a return value.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/wrap_08055700.c
 */

#include "gba_types.h"

extern u32 FUN_08055700(void);

/* 0x0803C044 */
u32 FUN_0803c044(void)
{
    return FUN_08055700();
}
