/* The SECOND copy of the decimal digit writer — 0x080674B0-0x08067523
 *
 * This function appears TWICE in the ROM. The md5 of the 116 bytes at
 * 0x080672FC and at this address is IDENTICAL (verified); that is, the same
 * source went into two separate translation units and the linker did not
 * deduplicate it.
 *
 * The body is the same as src/misc/format_decimal.c; only the function name
 * differs. The two must be recorded separately because they sit at separate
 * addresses in the ROM.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/misc/format_decimal_dup.c
 */

#include "gba_types.h"

#define POWER_COUNT 10

extern const u32 gPowersOfTen[];

/* 0x080674B0 */
void FormatDecimalDup(u32 value, u8 *out)
{
    s32 i;
    u8  digit;

    for (i = POWER_COUNT - 1; i >= 0; i--) {
        if (value >= gPowersOfTen[i])
            break;
    }
    if (i < 0)
        i = 0;

    do {
        digit = '0';
        while (value >= gPowersOfTen[i]) {
            value -= gPowersOfTen[i];
            digit++;
        }
        *out = digit;
        out++;
        i--;
    } while (i >= 0);

    *out = 0;
}
