/* Dorduncu argumani sifirlayarak iletme — 0x08038020-0x0803802B
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/forward_zero_arg4.c
 */

#include "gba_types.h"

extern void FUN_080374b0(u32 a, u32 b, u32 c, u32 d);

/* 0x08038020 */
void ForwardZeroArg4(u32 a, u32 b, u32 c)
{
    FUN_080374b0(a, b, c, 0);
}
