/* Set the pending code to 8 — 0x08005FA8-0x08005FBF
 *
 * Counterpart of FUN_08005F5C, which clears the same byte. The value 8 is
 * written twice over: once into gUnk02001428 and once as the first argument of
 * FUN_0803378C, and agbcc materialises it separately each time. The
 * placeholder name is kept: neither callee's purpose is recorded.
 *
 * FUN_0803378C is declared here with the two arguments the ROM passes.
 * src/world/wrap_0803378c.c declares it without any, because that wrapper
 * forwards whatever is already in r0-r3 and never names them; both spellings
 * describe the same entry point from different sides.
 *
 * Rule 35: `pop {r0}; bx r0` indicates a void return type.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/bootstrap/set_pending_code.c
 */

#include "gba_types.h"

extern u8 gUnk02001428;

extern void FUN_0803379c(void);
extern void FUN_0803378c(int a, int b);

/* 0x08005FA8 */
void FUN_08005fa8(void)
{
    FUN_0803379c();
    gUnk02001428 = 8;
    FUN_0803378c(8, 1);
}
