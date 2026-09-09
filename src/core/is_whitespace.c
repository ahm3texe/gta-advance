/* Is the character whitespace — 0x08008190-0x080081AD
 *
 * Returns 1 for carriage return, line feed, space and tab, 0 otherwise. The
 * parameter is normalized with lsls #24 / lsrs #24 on entry, so it is a u8
 * (rule 15).
 *
 * NOT YET MATCHING: 14 of the 15 instructions are correct and in the right
 * order. The whole difference is one instruction the ROM has and we do not:
 *     adds r1, r0, #0
 * a copy of the normalized argument made right after the normalization. The
 * ROM compares the first three values against r0 and the LAST against r1; we
 * compare all four against r0. Every branch target in the listing is then two
 * bytes earlier, which is why the comparison reports seven differences for one
 * missing instruction.
 *
 * The goto form below is what fixes the block layout: the ROM places the
 * `return 1` body after `return 0` and reaches it with four forward `beq`s
 * (rule 49). Written as `if (...) return 1; return 0;` the bodies swap.
 *
 * MEASURED, none of which keeps the copy alive: `u8 alt = c` used only in the
 * last test; the same as u32; the copy made after the third test; a u32
 * parameter narrowed twice into two locals; a `switch` over the four values
 * (11 differences, it builds a comparison chain of its own); a volatile copy
 * (15, it forces a stack slot). regmove coalesces the copy in every one of
 * them, and with no calls and no register pressure there is nothing to stop
 * it. This is the rule 44 class: the allocation is right, the source lever is
 * not known.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/is_whitespace.c
 */

#include "gba_types.h"

#define CHAR_TAB    9
#define CHAR_LF     10
#define CHAR_CR     13
#define CHAR_SPACE  32

/* 0x08008190 */
u32 IsWhitespace(u8 c)
{
    if (c == CHAR_CR) goto yes;
    if (c == CHAR_LF) goto yes;
    if (c == CHAR_SPACE) goto yes;
    if (c == CHAR_TAB) goto yes;
    return 0;
yes:
    return 1;
}
