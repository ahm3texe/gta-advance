/* HasWantedEntry — 0x08028E3C-0x08028E6B
 *
 * The first function scans 14 gEntriesA entries (stride 148) for active +0
 * and +100 == 101. The second is a five-function frame chain.
 *
 * BYTE-MATCHING. Keeping base separate from kind and cur preserves the ROM's
 * r0 base load and two separate pointer live ranges, also placing the literal
 * pool correctly.
 *
 * Sibling FrameChain: src/world/frame_chain.c
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/kind_scan.c
 */

#include "gba_types.h"

#define ENTRY_STRIDE  148
#define END_OFFSET    0x818
#define WANTED_KIND   101

typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01[99];
    u8  kind;                   /* +0x64 */
    u8  pad65[47];              /* stride 148 */
} Entry;

extern Entry gEntriesA[];

extern void FUN_08029130(void);
extern void FUN_08027f48(void);
extern void FUN_08019800(void);
extern void FUN_08019a64(void);
/* 0x08028E3C */
u32 HasWantedEntry(void)
{
    u8 *base;
    u8 *cur;
    u8 *kind;
    u8 *end;

    base = (u8 *)gEntriesA;
    kind = base + 100;
    cur  = base;
    end  = cur + END_OFFSET;

    do {
        if (*cur != 0) {
            if (*kind == WANTED_KIND)
                return 1;
        }
        kind += ENTRY_STRIDE;
        cur  += ENTRY_STRIDE;
    } while (cur <= end);

    return 0;
}
