/* Is the link ready — 0x0803CB1C-0x0803CB4B
 *
 * The IWRAM word at 0x0300009C is tested against bit 8 when byte 26 of
 * gUnk02010C60 is set, and against bit 0 when it is clear. Either test passing
 * answers 1.
 *
 * Both masks are materialised AFTER the byte test and anded INTO the word's
 * register, which is rule 33; the word itself is loaded first, before the byte.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/slot/is_link_ready.c
 */

#include "gba_types.h"

#define LINK_WORD  ((u32 *)0x0300009C)
#define MODE_BYTE  26
#define WIDE_BIT   (128 << 1)   /* 0x100 */
#define NARROW_BIT 1

extern u8 gUnk02010C60[];

/* 0x0803CB1C */
u32 FUN_0803cb1c(void)
{
    u32 word = *LINK_WORD;
    u32 mask;

    if (gUnk02010C60[MODE_BYTE] != 0) {
        mask = WIDE_BIT;
        word &= mask;
        if (word != 0) goto yes;
        goto no;
    }
    mask = NARROW_BIT;
    word &= mask;
    if (word == 0) goto no;
yes:
    return 1;
no:
    return 0;
}
