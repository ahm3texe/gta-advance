/* The entry's body, or nothing for 255 — 0x08028F98-0x08028FB7
 *
 * The index is sign-extended from 16 bits (`lsls #16 / asrs #16`), so it is an
 * s16, and 255 means "no entry". The address is computed BEFORE the test, which
 * is why the multiply and the base add come first in the ROM.
 *
 * 148 is the entry stride data/ram_map.csv already records for the table, and
 * +4 is the body past the leading active byte.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/entry_body_or_none.c
 */

#include "gba_types.h"

#define STRIDE  148
#define NONE    255

typedef struct Entry {
    u8  active;
    u8  pad01[99];
    u8  subActive;
    u8  pad65[47];
} Entry;

extern Entry gRam020246F0[20];

/* 0x08028F98 */
u8 *FUN_08028f98(s16 index)
{
    u8 *entry;

    entry = (u8 *)gRam020246F0 + index * STRIDE;
    if (index == NONE)
        return 0;
    return entry + 4;
}
