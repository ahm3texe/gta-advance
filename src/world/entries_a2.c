/* Searching gEntriesA by owner + state — 0x08028F4C-0x08028F97
 *
 * First validates the parameter with GetOwnerSlot; if the result is 0 it
 * returns 0 immediately.  If valid it looks at the 15 entries in gEntriesA
 * (stride 148) and searches for this triple: the entry is active (+0x00
 * non-zero), the +0x84 field equals the parameter, and the +0x90 field is 51
 * or 52.  It returns 1 if found and 0 if not.
 *
 * The ROM's `subs #51 / cmp #1 / bls` is a single range test; agbcc produces
 * it itself from the `x == 51 || x == 52` spelling (range canonicalisation).
 *
 * `pop {r4}; pop {r1}; bx r1` — the return address is taken into r1 because r0
 * stays live, i.e. the function returns a VALUE (the reverse of rule 35).
 *
 * BYTE-MATCHING (76/76).  Four things were needed together; each was measured:
 *
 * 1) A `for` WITH AN `s32 i` COUNTER AND ARRAY INDEXING (rules 9/31/42).  From
 *    the outside the ROM looks like pure pointer walking (`adds #148` +
 *    `cmp r1,r3`), but the branch is SIGNED (`ble`).  The hand-written pointer
 *    loop in the sibling kind_scan.c produces `bls`.  A signed branch can only
 *    come from an `i <= 14` comparison: after strength reduction agbcc
 *    eliminates the biv and moves the test onto the giv while PRESERVING the
 *    comparison's signedness.
 *
 * 2) A SINGLE POINTER FOR +0x84 AND +0x90 (EntryTail).  Writing the fields
 *    directly as `gEntriesA[i].unk84` / `gEntriesA[i].unk90` makes agbcc build
 *    THREE separate givs (base+0, base+132, base+144) and gives
 *    `push {r4,r5,r6,lr}`: 84 bytes / 63 off.  Collecting the two fields into
 *    one sub-structure and saying `tail = &base[i].tail` produced the ROM's
 *    two givs (r1 = +0, r2 = +0x84) and its `ldr [r2,#0]` / `ldr [r2,#12]`
 *    accesses: 76 bytes / 46 off.
 *
 * 3) THE `tail` ASSIGNMENT OUTSIDE THE `if`.  Written inside `if (active)`,
 *    the giv does not run on every round so agbcc does not strength-reduce it
 *    and rebuilds the address inside the loop as `r3 + r5`.  Moved ahead of
 *    the condition it became a giv.
 *
 * 4) `base` AS A SEPARATE LOCAL (rules 1/37).  Written as `gEntriesA[i]`
 *    directly, agbcc FOLDS the second giv's start into `.word gEntriesA+0x84`
 *    and recomputes the base with `subs #132` (72 bytes, 16 instructions off).
 *    The intermediate local `base = gEntriesA;` keeps the base in a register;
 *    with both givs derived from it, the ROM's
 *    `ldr r0,=base / adds r2,r0,#0 / adds r2,#132 / adds r1,r0,#0` sequence
 *    comes out.
 *
 * Its siblings: src/world/kind_scan.c (0x08028E3C, an unsigned pointer loop)
 * and src/world/entries_a4.c (0x08029244, an unreduced `i * 148`
 * multiplication).  Three different compiled forms of the same table.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_a2.c
 */

#include "gba_types.h"

#define ENTRY_COUNT_MAX  14         /* the loop runs 0..14, i.e. 15 rounds */
#define STATE_LOW        51
#define STATE_HIGH       52

typedef struct EntryTail {
    u32 owner;                      /* Entry +0x84 */
    u8  pad04[8];
    u32 state;                      /* Entry +0x90 */
} EntryTail;

typedef struct Entry {
    u8        active;               /* +0x00 */
    u8        pad01[131];
    EntryTail tail;                 /* +0x84, stride 148 */
} Entry;

extern Entry gEntriesA[];

extern u32 GetOwnerSlot(u32 arg);

/* 0x08028F4C */
u32 FindEntryByOwnerState(u32 arg)
{
    Entry *base;
    EntryTail *tail;
    s32 i;

    if (GetOwnerSlot(arg) == 0)
        return 0;

    base = gEntriesA;
    for (i = 0; i <= ENTRY_COUNT_MAX; i++) {
        tail = &base[i].tail;
        if (base[i].active != 0) {
            if (tail->owner == arg
             && (tail->state == STATE_LOW || tail->state == STATE_HIGH))
                return 1;
        }
    }

    return 0;
}
