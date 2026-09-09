/* Append one string to another — 0x080321EC-0x0803220B
 *
 * Standard strcat, except that the ROM leaves 0 in r0 on return rather than the
 * destination pointer, so it is written void.
 *
 * The first loop is entered at its test (`b` forward into the `ldrb`), which is
 * a while loop, not a do-while; the second is entered the same way.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/status/str_cat.c
 */

#include "gba_types.h"

/* 0x080321EC */
void FUN_080321ec(u8 *dest, const u8 *src)
{
    u8 c;

    while (*dest != 0)
        dest++;
    while ((c = *src) != 0) {
        *dest = c;
        dest++;
        src++;
    }
    *dest = 0;
}
