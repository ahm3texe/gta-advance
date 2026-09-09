/* Clear the pending code and set two selectors — 0x08005F5C-0x08005F6B
 *
 * Counterpart of FUN_08005FA8, which sets the same byte to 8. The two
 * gUnk02010C60 bytes are set to 3 together; byte 1 is the one menu_screen.c
 * reads back as `3 & ~gUnk02010C60[1]`, so 3 is its "nothing selected" value.
 * The placeholder name is kept: no reader of gUnk02001428 is known.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/bootstrap/clear_pending_code.c
 */

#include "gba_types.h"

extern u8 gUnk02001428;
extern u8 gUnk02010C60[];

/* 0x08005F5C */
void FUN_08005f5c(void)
{
    gUnk02001428 = 0;
    gUnk02010C60[1] = 3;
    gUnk02010C60[2] = 3;
}
