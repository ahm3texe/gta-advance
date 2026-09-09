/* Two stubs in the 0x080673FC-0x08067403 gap
 *
 * Neither was in data/functions.csv. They sit in the eight bytes between
 * BumpCount80 and IsKind20, which no entry covered, and were found by following
 * the `bl` at 0x0805A2C8 into the gap.
 *
 * 0x080673FC does nothing. Its only caller in the whole ROM is that one `bl`;
 * every Thumb `bl` in the image was scanned to establish that.
 *
 * 0x08067400 answers 0. It has NO caller at all by the same scan, so it is
 * either reached through a function pointer or is dead. Its address does not
 * appear as a Thumb-tagged pool word either.
 *
 * Both are four bytes, so they need no prologue and return through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/stub/gap_stubs.c
 */

#include "gba_types.h"

/* 0x080673FC */
void FUN_080673fc(void)
{
}

/* 0x08067400 */
u32 FUN_08067400(void)
{
    return 0;
}
