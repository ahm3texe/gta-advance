/* Call FUN_08034E48 and answer 0 — 0x080348B4-0x080348BF
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a return value, and the ROM sets
 * it AFTER the call, so the callee's answer is discarded deliberately --
 * src/glue/probe_ram_mode.c is the same shape.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/audio/call_and_zero.c
 */

#include "gba_types.h"

extern u32 FUN_08034e48(void);

/* 0x080348B4 */
u32 FUN_080348b4(void)
{
    FUN_08034e48();
    return 0;
}
