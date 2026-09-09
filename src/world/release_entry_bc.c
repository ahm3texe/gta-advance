/* Releasing a pool entry (pools B and C) — 0x08028B2C-0x08028B9B
 *
 * The two functions are identical INSTRUCTION BY INSTRUCTION; only the base
 * table differs.  If the entry is not active it returns 0; if it is, it hands
 * the sub-object to ReleaseObject, zeroes +0x00, +0x8C and +0x02, and
 * returns 1.
 *
 * Because +0x8C exceeds Thumb's `str` immediate range (124 at most), the ROM
 * produces a separate base (adds r1,r4,#0 / adds r1,#140); that falls out of
 * the natural field write in C on its own.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/release_entry_bc.c
 */

#include "gba_types.h"

/* The same 148 = 0x94 stride as in the sibling files (entries_b1.c,
 * entries_b5.c); the fields these two functions touch have been named. */
typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01;
    u16 timer;                  /* +0x02 */
    u8  sub[38];                /* +0x04, passed to ReleaseObject */
    u8  pad2a[98];
    u32 unk8c;                  /* +0x8C */
    u8  pad90[4];
} Entry;

extern Entry gRam02024350[];    /* 4 entries */
extern Entry gRam020245B0[];    /* 1 entry */

extern void ReleaseObject(u8 *sub);

/* 0x08028B2C */
u32 ReleaseEntryB(u8 idx)
{
    Entry *e;

    e = &gRam02024350[idx];
    if (e->active == 0)
        return 0;

    ReleaseObject(e->sub);
    e->active = 0;
    e->unk8c = 0;
    e->timer = 0;
    return 1;
}

/* 0x08028B64 */
u32 ReleaseEntryC(u8 idx)
{
    Entry *e;

    e = &gRam020245B0[idx];
    if (e->active == 0)
        return 0;

    ReleaseObject(e->sub);
    e->active = 0;
    e->unk8c = 0;
    e->timer = 0;
    return 1;
}
