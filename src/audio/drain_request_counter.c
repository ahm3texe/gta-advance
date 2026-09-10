/* Drain the request counter — 0x08033B74-0x08033B9F
 *
 * A counter of 2 and a counter of 1 both call the SAME routine once, and then
 * the counter is cleared either way. Two calls, not a loop: the ROM has two
 * separate `bl` sites and the byte is re-read from the pool for the store.
 *
 * The two tests are not an else-chain: the first branches past the second, and
 * a value of 2 therefore calls once and skips the 1 test.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/audio/drain_request_counter.c
 */

#include "gba_types.h"

extern u8 gRam02027314;

extern void FUN_080333ac(void);

/* 0x08033B74 */
void FUN_08033b74(void)
{
    u8 pending = gRam02027314;

    if (pending == 2)
        FUN_080333ac();
    else if (pending == 1)
        FUN_080333ac();
    gRam02027314 = 0;
}
