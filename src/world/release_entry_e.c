/* Release a pool entry (pool E) — 0x080294B4-0x080294E3
 *
 * Instruction-for-instruction identical to ReleaseEntryD (release_entry_d.c)
 * except for the base table gRam020254D0. tools/find_twins.py reported 95.8%
 * similarity; copied the sibling source and changed the base (WORKFLOW.md §10).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/release_entry_e.c
 */

#include "gba_types.h"

typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01;
    u16 timer;                  /* +0x02 */
    u8  sub[38];                /* +0x04, passed to ReleaseObject */
    u8  pad2a[106];             /* total 148 = 0x94 */
} Entry;

extern Entry gRam020254D0[];    /* 5 entries */

extern void ReleaseObject(u8 *sub);

/* 0x080294B4 */
u32 ReleaseEntryE(u8 idx)
{
    Entry *e;

    e = &gRam020254D0[idx];
    if (e->active == 0)
        return 0;

    ReleaseObject(e->sub);
    e->active = 0;
    e->timer = 0;
    return 1;
}
