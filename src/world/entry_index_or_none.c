/* One-based handle to a zero-based index — 0x08031FF0-0x08032005
 *
 * The +0x16 halfword stores an index biased by one so that zero can mean "no
 * entry". This turns it back into a plain index, answering -1 for the zero
 * handle and for anything past the 128-entry limit. The placeholder name is
 * kept: the table the index addresses was not identified.
 *
 * `value - 1` is truncated back to 16 bits (`lsls #16 / lsrs #16`) purely
 * because the difference has type u16; the subtraction cannot wrap here, since
 * the zero case has already returned.
 *
 * Rule 49: both -1 answers share one body at the end of the function.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entry_index_or_none.c
 */

#include "gba_types.h"

#define ENTRY_LIMIT  128

typedef struct HandleOwner {
    u8  pad00[0x16];
    u16 handle;                 /* +0x16, one-based; 0 means no entry */
} HandleOwner;

/* 0x08031FF0 */
s32 FUN_08031ff0(HandleOwner *owner)
{
    u16 handle;
    u16 index;

    handle = owner->handle;
    if (handle == 0) goto none;
    index = handle - 1;
    if (index > ENTRY_LIMIT - 1) goto none;
    return index;
none:
    return -1;
}
