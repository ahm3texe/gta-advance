/* Write the same word to +0x08 and +0x0C — 0x08055768-0x08055773
 *
 * gUnk02010C60 is declared `u8 []` because that is the type its other users
 * give it (src/ui/menu_screen.c and its siblings read single bytes); the
 * consistency check requires one type per symbol, so the two words are reached
 * through a local u32 view.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/set_pair_words.c
 */

#include "gba_types.h"

extern u8 gUnk02010C60[];

/* 0x08055768 */
void FUN_08055768(u32 value)
{
    u32 *words = (u32 *)gUnk02010C60;

    words[2] = value;
    words[3] = value;
}
