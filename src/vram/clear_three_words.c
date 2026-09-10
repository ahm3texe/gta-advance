/* Clear three separate words — 0x08011D50-0x08011D6B
 *
 * Rule 67: the three addresses are LOCALS. The ROM materialises all three
 * before the zero and stores them in the reverse of the order it loaded them,
 * which is the shape address locals give -- their declaration order is the pool
 * order and their use order is the store order.
 *
 * Two other spellings were measured. Three plain assignments load each address
 * at its own store and interleave them (5/14). A chained
 * `gRam0201AEC0 = gRam0201AEC8 = gRam0201AEE4 = 0;` gets the shape right but
 * not the register assignment (12/14).
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/vram/clear_three_words.c
 */

#include "gba_types.h"

extern u32 gRam0201AEC0;
extern u32 gRam0201AEC8;
extern u32 gRam0201AEE4;

/* 0x08011D50 */
void FUN_08011d50(void)
{
    u32 *third = &gRam0201AEE4;
    u32 *second = &gRam0201AEC8;
    u32 *first = &gRam0201AEC0;

    *first = 0;
    *second = 0;
    *third = 0;
}
