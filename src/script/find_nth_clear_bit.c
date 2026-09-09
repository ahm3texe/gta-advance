/* Index of the n-th clear bit of a 32-bit mask — 0x0805B53C-0x0805B567
 *
 * Walks bits 0 to 31, counting only the CLEAR ones, and answers the index of
 * the one whose count reaches n. Answers 32 when the mask has fewer than n+1
 * clear bits, which is one past the last valid index rather than an error code.
 *
 * Not a script handler: it takes two real arguments and its answer is an index,
 * not a success flag. It sits among the handlers only because of where the
 * linker put it.
 *
 * The constant 1 is a local, which is what puts it in a callee-saved register
 * hoisted out of the loop and copied into the shift's register each turn
 * (`movs r5,#1` once, then `adds r0,r5,#0 / lsls r0,r2` inside). Written
 * `1 << bit` the constant is rebuilt every iteration.
 *
 * The loop bound is a SIGNED comparison (`cmp r2,#31 / ble`), so the index is
 * an int rather than one of the unsigned types.
 *
 * The index is initialised as its own statement rather than in a `for` header,
 * because the ROM zeroes it BEFORE the counter and the constant. In a `for` the
 * initialiser is emitted after the two declarations that precede the loop, and
 * the three `movs` come out in the wrong order.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/find_nth_clear_bit.c
 */

#include "gba_types.h"

#define BIT_COUNT  32

/* 0x0805B53C */
s32 FindNthClearBit(u32 mask, u32 wanted)
{
    s32 bit;
    u32 seen;
    u32 one;
    u32 probe;

    bit = 0;
    seen = 0;
    one = 1;
    while (bit <= BIT_COUNT - 1) {
        probe = one;
        probe <<= bit;
        probe &= mask;
        if (probe == 0) {
            if (seen == wanted)
                return bit;
            seen++;
        }
        bit++;
    }
    return BIT_COUNT;
}
