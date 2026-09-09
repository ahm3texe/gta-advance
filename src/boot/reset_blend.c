/* Reset the blend registers — 0x08063B74-0x08063B97
 *
 * BLDCNT gets 0xFF, BLDALPHA 0, BLDY 31, and gRam02036110 is cleared first.
 *
 * BLDY is reached by ADVANCING the BLDCNT pointer (`adds r1,#4`) rather than
 * from its own pool word, while BLDALPHA gets one of its own.
 *
 * The pointer has to be `volatile` for that. A plain `u16 *` starting at a
 * constant address is itself a constant, so agbcc folds `reg += 2` into a
 * displacement and writes `strh r0,[r1,#4]`; the advance disappears and rule 64
 * never gets a chance to apply. `volatile` stops the fold, and only then does
 * the ROM's separate `adds r1,#4` appear. Six spellings were measured and this
 * is the only one that produces it.
 *
 * The zero written to BLDALPHA is the same register as the one written to
 * gRam02036110; the ROM materialises it AFTER that symbol's address, so the
 * address goes through a local of its own and the zero is assigned second.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/boot/reset_blend.c
 */

#include "gba_types.h"
#include "gba_io.h"

#define BLEND_ALL   0xFF
#define BLEND_FULL  31

extern u32 gRam02036110;

/* 0x08063B74 */
void FUN_08063b74(void)
{
    u32 *state;
    volatile u16 *reg;
    u32 zero;

    state = &gRam02036110;
    zero = 0;
    *state = zero;
    reg = (volatile u16 *)REG_BLDCNT_ADDR;
    *reg = BLEND_ALL;
    REG_BLDALPHA = zero;
    reg += 2;
    *reg = BLEND_FULL;
}
