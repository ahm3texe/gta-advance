/* The anchor's +0x08 word as a flag — 0x08051470-0x0805147F
 *
 * Rule 48: the answer is the getter's own value, replaced by 1 when it is not
 * zero, so a zero needs no store at all -- the ROM has no `movs r0,#0`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/anchor_unk08_as_flag.c
 */

#include "gba_types.h"

extern u32 FUN_08050970(void);

/* 0x08051470 */
u32 FUN_08051470(void)
{
    u32 value = FUN_08050970();

    if (value != 0)
        value = 1;
    return value;
}
