/* The low two bits of the IWRAM word at 0x03000098 — 0x0803CBB4-0x0803CBC3
 *
 * The address comes through the literal pool as a constant, so it is a cast and
 * not an extern (rule 1 applies to the mapped RAM symbols).
 *
 * Rule 33: the 3 is materialised after the load and the word anded INTO it.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/slot/read_iwram_low_two_bits.c
 */

#include "gba_types.h"

#define IWRAM_WORD  ((u32 *)0x03000098)

/* 0x0803CBB4 */
u32 FUN_0803cbb4(void)
{
    u32 value = *IWRAM_WORD;
    u32 mask = 3;

    value &= mask;
    return value;
}
