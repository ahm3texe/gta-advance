/* The table's default handler — 0x0805AB14-0x0805AB17
 *
 * Four bytes that answer 1. It was in no map entry, in the gap between
 * 0x0805AB04 and 0x0805AB18, and it has NO `bl` caller anywhere in the ROM.
 *
 * It is reached only as a Thumb function pointer. Its tagged address
 * 0x0805AB15 appears 27 times in the image: once in the literal pool of
 * 0x080556C0, and 26 times as the +0x00 field of 20-byte entries in three
 * tables at 0x08CFAF04, 0x08D0D99C and 0x08D13DBC. Those entries are otherwise
 * identical to each other, so this is the "nothing to do, succeed" handler that
 * fills a table slot.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/table/always_one.c
 */

#include "gba_types.h"

/* 0x0805AB14 */
u32 FUN_0805ab14(void)
{
    return 1;
}
