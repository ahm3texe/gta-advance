/* Draw submission for every node of the two-layer entry list
 * 0x0801515C-0x080151BF, 100 bytes  [MATCHED]
 *
 * What the ROM does (the same traversal skeleton as the sibling
 * ReleaseEntryResources):
 *   Starting from the head pointer at 0x020230A0, it walks the outer list
 *   linked through +0x3C.  Each outer node is at the same time the HEAD of
 *   the inner list linked through +0x44.  For every node in the inner list,
 *   if the +0x30 field is non-empty:
 *     node->unk1c = AllocDrawEntry(node->unk10, (w * h) >> 1,
 *                                w >> 3, h >> 3, node->flags20);
 *     if (node->kind27 == 1) node->flags20 |= 0x2000;
 *   Here w = the +0x14 and h = the +0x15 byte fields; they behave like pixel
 *   dimensions: divided by 8 they give the tile count, multiplied together
 *   and halved they give the 4bpp byte size.  Unlike in the sibling file,
 *   +0x30 does not go through a RANGE filter here, it is only checked for
 *   zero.
 *
 * DETAILS READ FROM THE ROM
 * -------------------------
 * (1) The +0x30 CHECK comes AFTER the multiplication: the r6/r2/r3 loads and
 *     the `muls`/`asrs` sit above the branch.  That is why the loads and the
 *     half-size computation are separate statements BEFORE the `if` in the
 *     source too.  Measured: moving `half` inside the `if` leaves 20 bytes
 *     off and moving `src` inside leaves 26 bytes off -- agbcc does not move
 *     the loads above the branch.
 * (2) THE W AND H LOCALS MUST BE u8 -- THE ONE MEASUREMENT THAT OPENED THE
 *     MATCH.  When they are written as u32 the instruction stream comes out
 *     LITERALLY the same, only r5 and r6 swap places (8 bytes): `outer` falls
 *     into r6 and `src` into r5, the reverse of the ROM.  So the rule 50
 *     lever works here not directly through `outer`/`src` but through the
 *     lifetimes of two neighboring narrow-typed pseudos; the narrow type
 *     flips the allocation order and `outer` gets allocated first and takes
 *     r5.  A mixture of u8/u32 (one narrow, the other wide) is again 8 bytes,
 *     and s32 gives 10 bytes off.
 * (3) The signedness conflict is only apparent: for the same r2/r3 the ROM
 *     uses both `asrs r1,r0,#1` (signed) and `lsrs r2,r2,#3` (unsigned).
 *     Because the u8 local promotes to int, `(w * h) >> 1` gives a signed
 *     shift, while `w >> 3` is simplified to a logical shift since the upper
 *     bits of the value are known to be zero.  No forcing is needed in the
 *     source: an `(s32)` cast and writing `/ 2` give the same bytes as well,
 *     and the plainest one was kept.
 * (4) The fifth argument goes through the stack (`sub sp,#4` +
 *     `str r0,[sp,#0]`), is read with ldrh and written as 32 bits: the
 *     signature takes the last parameter as u16 (writing u32 gives the same
 *     bytes too, but the narrow type is more faithful to the ROM).
 * (5) Since the +0x27 offset exceeds the ldrb immediate limit (#31), agbcc
 *     takes the address into a separate register on its own; there is NO
 *     pointer local in the source (it had been measured this way in the
 *     sibling file too).
 * (6) Both loops are ENTRY-GUARDED + bottom-testing in shape; the inner
 *     loop's guard tests the outer variable (r5), because after the
 *     `node = outer;` copy the condition is written through outer via CSE.
 *     Rule 49: there is no cold body at the end of the function.
 *
 * SPELLINGS TRIED AND REJECTED
 * ----------------------------
 * - `u32 w, h` (and the u8/u32 mixture, `s32`): the instruction stream is the
 *   same, r5<->r6 are reversed, 8-10 bytes off.  See (2) above.
 * - The rule 33 shape (`bits = 0x2000; bits |= flags20; flags20 = bits;`):
 *   it DELETES the constant copy in the ROM (`adds r0,r1,#0`), giving
 *   96 bytes.  In this function the correct writing is a plain `|=`; rule 33
 *   is not universal.
 * - Moving `src` inside the `if` (26 bytes), moving `half` inside (20 bytes).
 * - Ones that had no effect (again 8 bytes, i.e. they do not flip the
 *   allocation): permutations of the declaration order, the two loops in
 *   `for` shape, an explicit `if (outer != 0)` guard for the inner loop, a
 *   separate `head` local (a rule 22 attempt), writing `src` as `u8 *`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/level_step_b2.c
 */

#include "gba_types.h"

/* Submission routine: source, 4bpp byte size, tile dimensions and flags.
   The signature was read from the ROM's call setup; the function itself
   (0x08012E78) has not been decompiled yet. */
void *AllocDrawEntry(void *src, s32 size, u32 tilesX, u32 tilesY, u16 flags);

typedef struct Entry {
    u8            pad00[0x10];
    void         *unk10;        /* +0x10 first argument of AllocDrawEntry */
    u8            width14;      /* +0x14 pixel width */
    u8            height15;     /* +0x15 pixel height */
    u8            pad16[6];
    void         *unk1c;        /* +0x1C the return of the call is written here */
    u16           flags20;      /* +0x20 */
    u8            pad22[5];
    u8            kind27;       /* +0x27 */
    u8            pad28[8];
    u32           unk30;        /* +0x30 non-empty check */
    u8            pad34[8];
    struct Entry *next3c;       /* +0x3C outer list link */
    u8            pad40[4];
    struct Entry *next44;       /* +0x44 inner list link */
} Entry;

/* The flag set at +0x20; its meaning is unresolved, the value came from the ROM. */
#define FLAG_SUBMITTED 0x2000

/* 0x020230A0: the head pointer of the outer list.  The rationale is the same
   as in the sibling file: since the offset is 0, rule 1's folding problem does
   not arise, and the ROM likewise reads the address from the pool in one piece
   and does `ldr r5,[r0,#0]`.  A data/ram_map.csv record is needed for this
   address; I do not have the authority to define symbols. */
#define gListHead020230A0 (*(Entry **)0x020230A0)

/* 0x0801515C */
void LoadEntryTileData(void)
{
    Entry *outer;
    Entry *node;
    void *src;
    u8 w;                       /* narrow type is mandatory -- header item (2) */
    u8 h;
    s32 half;

    outer = gListHead020230A0;
    while (outer != 0) {
        node = outer;
        while (node != 0) {
            src = node->unk10;
            w = node->width14;
            h = node->height15;
            half = (w * h) >> 1;
            if (node->unk30 != 0) {
                w >>= 3;
                h >>= 3;
                node->unk1c = AllocDrawEntry(src, half, w, h, node->flags20);
                if (node->kind27 == 1) {
                    node->flags20 |= FLAG_SUBMITTED;
                }
            }
            node = node->next44;
        }
        outer = outer->next3c;
    }
}
