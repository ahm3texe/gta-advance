/* Sifir arguman ekleyerek iletme — 0x08030874-0x08030883
 *
 * Ucuncu argumani dorduncu siraya kaydirip araya sifir koyarak
 * FUN_0802B1E4'u cagiriyor (ROM: `adds r3,r2,#0` sonra `movs r2,#0`).
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/forward_with_zero.c
 */

#include "gba_types.h"

extern void FUN_0802b1e4(u32 a, u32 b, u32 c, u32 d);

/* 0x08030874 */
void ForwardWithZero(u32 a, u32 b, u32 c)
{
    FUN_0802b1e4(a, b, 0, c);
}
