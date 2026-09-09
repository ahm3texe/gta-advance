/* Tear down the first entry — 0x0804283C-0x08042863
 *
 * The +0x00 word of the argument holds the entry; when there is one it is run
 * through two teardown routines and then handed to FUN_0800C804 together with
 * the address of gRam020110C0, which data/ram_map.csv records as being passed
 * that way by ReleaseAreaNode too.
 *
 * The entry stays in r4 across all three calls, which is what the `push {r4}`
 * pays for.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/map/release_first_entry.c
 */

#include "gba_types.h"

typedef struct EntryHolder {
    u32 entry;                  /* +0x00 */
} EntryHolder;

extern u32 gRam020110C0[];

extern void FUN_080428ac(u32 entry);
extern void FUN_08042900(u32 entry);
extern void FUN_0800c804(u32 *dest, u32 value);

/* 0x0804283C */
void FUN_0804283c(EntryHolder *holder)
{
    u32 entry = holder->entry;

    if (entry == 0)
        return;
    FUN_080428ac(entry);
    FUN_08042900(entry);
    FUN_0800c804(gRam020110C0, entry);
}
