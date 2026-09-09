/* Decimal digit writer — 0x080672FC-0x0806736F
 *
 * Converts a number into ASCII digits without leading zeroes. There is no
 * division: it performs repeated subtraction using the 10^0..10^9 table at
 * 0x08FD17FC (in agbcc a `/` would emit __divsi3, and the ROM has no such
 * call).
 *
 * First the highest power covering the value is found, then, working down from
 * that digit, the subtraction count for each power is added on top of '0'.
 * The digit is u8: the ROM truncates it with a 24-bit shift after every
 * increment.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/misc/format_decimal.c
 */

#include "gba_types.h"

#define POWER_COUNT 10

extern const u32 gPowersOfTen[];

/* 0x080672FC */
void FormatDecimal(u32 value, u8 *out)
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
