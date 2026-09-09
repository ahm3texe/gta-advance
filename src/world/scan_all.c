/* Process the first table entry — 0x08029014-0x08029051
 *
 * After incrementing, the ROM loops back only if i == 0. Since i starts at
 * zero, only entry 0 is processed; this does not scan 65536 entries. If the
 * first entry is active, subActive selects NoOp080289B8 or NoOp080289B4.
 *
 * BYTE-MATCHING: an explicit integer addition placing the product before
 * the pointer base preserves the ROM's adds r1,r0,r5 operand order.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/scan_all.c
 */

#include "gba_types.h"

typedef struct Entry {
    u8  active;
    u8  pad01[99];
    u8  subActive;
    u8  pad65[47];
} Entry;

extern Entry gRam02024650[];

extern u32 NoOp080289B8(u16 i);
extern u32 NoOp080289B4(u16 i);

/* 0x08029014 */
void ProcessFirstEntry(void)
{
    u16 i;
    Entry *e;
    Entry *tbl;

    i = 0;
    tbl = gRam02024650;
    do {
        /* Operand order matters for byte-matching. */
        e = (Entry *)((u32)i * sizeof(Entry) + (u32)tbl);
        if (e->active != 0) {
            if (e->subActive == 0)
                NoOp080289B8(i);
            else
                NoOp080289B4(i);
        }
        i++;
    } while (i == 0);
}
