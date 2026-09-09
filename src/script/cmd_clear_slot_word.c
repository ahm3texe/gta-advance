/* Script command: clear the +0x4C word of slot 1 — 0x0805B9E4-0x0805B9F5
 *
 * SelectSlotAB's answer is used without a null check here, unlike in
 * src/ui/init_menu_screen.c and src/world/p1_23778.c, which both test it. The
 * ROM has no test; it is not an omission in the transcription.
 *
 * The slot is viewed as u32 here so that +0x4C is index 19. Other users of
 * SelectSlotAB give it a byte view for their own byte offsets; the function's
 * return type is not covered by the one-type-per-symbol check, which applies to
 * RAM symbols.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_clear_slot_word.c
 */

#include "gba_types.h"

#define SLOT_WORD  19           /* +0x4C */

extern u32 *SelectSlotAB(u32 which);

/* 0x0805B9E4 */
u32 FUN_0805b9e4(void)
{
    u32 *slot = SelectSlotAB(1);

    slot[SLOT_WORD] = 0;
    return 1;
}
