/* Unpack four 7-bit digits and start — 0x080324FC-0x0803251B and
 * 0x0803251C-0x08032547
 *
 * The first reads four bytes and packs them at 7 bits each, low byte first:
 * b0 + (b1 << 7) + (b2 << 14) + (b3 << 21). Seven bits per byte, not eight, so
 * each source byte carries a value below 128 and the four together make 28 bits.
 *
 * The second writes the three words the pair exists to set. The mask 0x3FF is
 * used twice and is a local for that reason: the ROM keeps it in r3 across both
 * stores rather than rebuilding it.
 *
 * The second word is `1 - value`, not `value - 1`; the ROM's `movs r1,#1 / subs
 * r1,r1,r0` gives the order away, and the mask makes the difference visible for
 * a value above 1.
 *
 * Rule 67: the first destination is an address local, and the shift and the
 * mask are locals of their own. That is what puts the ROM's three loads in the
 * order it has them, the destination first and the mask after the shift; with
 * the mask taken at the top it is hoisted above the shift instead.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/status/unpack_and_start.c
 */

#include "gba_types.h"

#define DIGIT_BITS  7
#define FIELD_MASK  0x3FF

extern u32 gRam02027300;
extern u32 gRam020272F8;
extern u32 gRam020272F4;

/* 0x0803251C */
void FUN_0803251c(s32 value);

/* 0x080324FC */
void FUN_080324fc(const u8 *digits)
{
    s32 value;

    value = digits[0] + (digits[1] << DIGIT_BITS);
    value = value + (digits[2] << (DIGIT_BITS * 2));
    value = value + (digits[3] << (DIGIT_BITS * 3));
    FUN_0803251c(value);
}

/* 0x0803251C */
void FUN_0803251c(s32 value)
{
    u32 *dest = &gRam02027300;
    u32 shifted;
    u32 mask;

    shifted = value << 4;
    mask = FIELD_MASK;
    *dest = shifted & mask;
    gRam020272F8 = (1 - value) & mask;
    gRam020272F4 = 0;
}
