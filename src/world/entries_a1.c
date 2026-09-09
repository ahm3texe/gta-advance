/* HasActiveEntryOfKind — 0x08028DF4-0x08028E39
 *
 * Scans the gEntriesA table (15 entries, 148-byte stride) from start to end.
 * If an entry is active (+0x00 non-zero) it checks whether any of three
 * conditions holds: is the kind field (+0x64) 0x33 or 0x4C, or is the word at
 * +0x90 equal to 46.  If any holds it returns 1, if none does it returns 0.
 *
 * The same pattern as its sibling HasWantedEntry (0x08028E3C,
 * src/world/kind_scan.c): the base is held in its own local and three walking
 * pointers are derived from it (rule 37).  The setup order in the ROM is
 * exactly the source order: +0x90, +0x64, base, end.
 *
 * In the ROM the end pointer is built FROM THE BASE with `adds r4, r2, r0`
 * (pool constant 0x8A8), while the comparison uses the +0x90 walker; hence
 * `end = cur + 0x8A8`.
 *
 * The prologue `push {r4, lr}` -> five live values (the base, three walkers,
 * the end), consistent with the register table in docs/COMPILER.md.  The
 * return `pop {r4}; pop {r1}; bx r1` with r0 live -> it returns a VALUE (the
 * reverse of rule 35, the same as alloc_node.c).
 *
 * MATCH: 70/70 bytes, on the first attempt.  The sibling function's pattern
 * (rule 37 + preserving the source order, rule 19) fitted directly; no variant
 * attempts were needed.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_a1.c
 */

#include "gba_types.h"

#define ENTRY_STRIDE  148
#define END_OFFSET    0x8A8

#define KIND_A        0x33
#define KIND_B        0x4C
#define EXTRA_WANTED  46

/* The same layout as Entry in src/world/kind_scan.c; the +0x90 field was
 * added here because this is where it first appears (the stride is still
 * 148). */
typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01[99];
    u8  kind;                   /* +0x64 */
    u8  pad65[43];
    u32 extra;                  /* +0x90 */
} Entry;

extern Entry gEntriesA[];

/* 0x08028DF4 */
u32 HasActiveEntryOfKind(void)
{
    u8 *base;
    u8 *extra;
    u8 *kind;
    u8 *cur;
    u8 *end;

    base  = (u8 *)gEntriesA;
    extra = base + 0x90;
    kind  = base + 0x64;
    cur   = base;
    end   = cur + END_OFFSET;

    do {
        if (*cur != 0) {
            if (*kind == KIND_A || *kind == KIND_B ||
                *(u32 *)extra == EXTRA_WANTED)
                return 1;
        }
        extra += ENTRY_STRIDE;
        kind  += ENTRY_STRIDE;
        cur   += ENTRY_STRIDE;
    } while (extra <= end);

    return 0;
}
