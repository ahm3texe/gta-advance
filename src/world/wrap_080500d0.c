/* Forwarding wrapper — 0x080500D0-0x080500D9
 *
 * The body only calls FUN_0804bfbc. Its purpose is unknown, so its name
 * is unchanged. Found with tools/find_wrappers.py.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a RETURN VALUE.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/wrap_080500d0.c
 */

#include "gba_types.h"

extern u32 FUN_0804bfbc(void);

/* 0x080500D0 */
u32 FUN_080500d0(void)
{
    return FUN_0804bfbc();
}
