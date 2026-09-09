/* Release a pool entry (pool D) — 0x08028DC4-0x08028DF3
 *
 * Same template as ReleaseEntryB/C (release_entry_bc.c), except +0x8C is
 * NOT CLEARED here, so the Entry view is shallower.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/release_entry_d.c
 */

#include "gba_types.h"

typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01;
    u16 timer;                  /* +0x02 */
    u8  sub[38];                /* +0x04, passed to ReleaseObject */
    u8  pad2a[106];             /* total 148 = 0x94 */
} Entry;

extern Entry gRam02023710[];    /* 5 entries */

extern void ReleaseObject(u8 *sub);

/* 0x08028DC4 */
u32 ReleaseEntryD(u8 idx)
{
    Entry *e;

    e = &gRam02023710[idx];
    if (e->active == 0)
        return 0;

    ReleaseObject(e->sub);
    e->active = 0;
    e->timer = 0;
    return 1;
}
