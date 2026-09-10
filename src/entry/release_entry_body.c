/* Release the entry's body if it is active — 0x08029058-0x08029087
 *
 * Answers 1 when there was something to release and 0 when the entry's leading
 * active byte was already clear. The +0x8C word is cleared from the same zero
 * as the active byte, through a pointer add because 140 is past Thumb's reach.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/release_entry_body.c
 */

#include "gba_types.h"

#define STRIDE       148
#define TAIL_OFFSET  140

typedef struct Entry {
    u8  active;
    u8  pad01[99];
    u8  subActive;
    u8  pad65[47];
} Entry;

extern Entry gRam02024650[];

extern void ReleaseObject(u32 *block);

/* 0x08029058 */
u32 FUN_08029058(u32 index)
{
    u8 *entry = (u8 *)gRam02024650 + index * STRIDE;
    u32 zero;
    u32 *tail;

    if (*entry == 0)
        return 0;
    ReleaseObject((u32 *)(entry + 4));
    zero = 0;
    *entry = zero;
    tail = (u32 *)(entry + TAIL_OFFSET);
    *tail = zero;
    return 1;
}
