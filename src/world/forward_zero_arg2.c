/* Ikinci argumani sifirlayarak iletme — 0x08055BF8-0x08055C03
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/forward_zero_arg2.c
 */

#include "gba_types.h"

extern void ReleaseAreaNode(u32 a, u32 b);

/* 0x08055BF8 */
void ForwardZeroArg2(u32 a)
{
    ReleaseAreaNode(a, 0);
}
