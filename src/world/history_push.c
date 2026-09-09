/* Push onto the history queue — 0x08008064-0x08008093
 *
 * gHistory holds the current record at +0 and a shifted 31-entry history at
 * +0x78. If the new record differs, shift the queue by one and update current.
 * The shift walks BACKWARDS from high to low addresses, overwriting the final
 * entry as it proceeds.
 *
 * BYTE-MATCHING. Reading current separately and copying the base with h2 = h
 * before comparison preserves the ROM's r0 -> r4 live range and prevents the
 * equality branch from merging with the final store.
 *
 * Sibling SetIndexReturnOne: src/world/set_index.c
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/history_push.c
 */

#include "gba_types.h"

#define HISTORY_LAST   30
#define HISTORY_HEAD   0x78

typedef struct History {
    u32 current;                /* +0x00 */
    u8  pad04[HISTORY_HEAD - 4];
    u32 slots[31];              /* +0x78 */
} History;

extern History gHistory;

/* 0x08008064 */
void PushHistory(u32 value)
{
    History *h;
    History *h2;
    u32     *cur;
    u32      current;
    s32      i;

    if (value == 0)
        return;

    h = &gHistory;
    current = h->current;
    h2 = h;
    if (value == current)
        return;

    i = HISTORY_LAST;
    cur = h2->slots;

    do {
        cur[1] = cur[0];
        cur--;
        i--;
    } while (i >= 0);

    h2->current = value;
}
