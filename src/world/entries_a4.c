/* Find a four-condition match in gEntriesA — 0x08029244-0x08029299
 *
 * Scan 15 entries (stride 148) for: active at +0x00, +0x84 equals the first
 * parameter, type +0x64 is 34, and +0x90 equals the second parameter. Return
 * 1 on a match, otherwise 0.
 *
 * BYTE-MATCHING on the first attempt. The ROM recomputes i*148 each iteration;
 * plain array indexing is correct, unlike the walking pointer in kind_scan.c.
 * Four fields need four bases, so agbcc leaves the multiplication in the loop
 * rather than applying strength reduction. The counter is s32 because the
 * ROM uses `cmp r1,#14` followed by signed ble (rules 9/31).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_a4.c
 */

#include "gba_types.h"

#define WANTED_KIND   34

typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01[99];
    u8  kind;                   /* +0x64 */
    u8  pad65[31];
    u32 unk84;                  /* +0x84 */
    u8  pad88[8];
    u32 unk90;                  /* +0x90, stride 148 */
} Entry;

extern Entry gEntriesA[];

/* 0x08029244 */
u32 FindEntryByQuad(u32 a, u32 b)
{
    s32 i;

    for (i = 0; i <= 14; i++) {
        if (gEntriesA[i].active != 0
         && gEntriesA[i].unk84 == a
         && gEntriesA[i].kind == WANTED_KIND
         && gEntriesA[i].unk90 == b)
            return 1;
    }

    return 0;
}
