/* Focus point update - 0x0800A9E4-0x0800AA3B  (88 bytes, MATCHED)
 *
 * If the gGameState +0x0C flag is set AND a second target exists, it writes
 * the MIDPOINT of the two targets' positions into gFocusPoint; otherwise the
 * first target's position. The division is `asrs #1`, i.e. signed.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 * The CoordBlock definition must be BYTE-FOR-BYTE the same as in
 * src/misc/coord_accessors.c; +0x04 and +0x08 are cast to target pointers.
 *
 * The trace log (docs/GAME_FLOW.md) MEASURED gFocusPoint to be two 16.16
 * fixed-point s32 values (starting at 3360.000 / 9568.000; some differences
 * are exactly 65536 and 262144). That VERIFIED the `Vec2 {s32 x; s32 y;}`
 * definition used here.
 *
 * ================= THE TWO MEASUREMENTS THAT CLOSED THE GAP ==============
 *
 * (1) RULE 45 -- SEPARATE LOCALS PER BRANCH.  77 -> 26 bytes of difference.
 *     The earlier form used the same `out` and `first` locals in both
 *     branches.  The last instruction of the two branches (`str r0,[out+4]`)
 *     was therefore the SAME insn in RTL, and the `jump` pass merged them by
 *     CROSS-JUMPING: the output stayed at 84 bytes (the ROM has 88) and the
 *     fall-through branch jumped to the final store with a `b`.  Giving the
 *     branches separate locals (`out`/`mout`, `first`/`mfirst`) made the merge
 *     impossible; the size rose to 88 and THE REGISTER ALLOCATION also settled
 *     onto the ROM's by itself (block r2->r3, out r4->r2, the position pointer
 *     r0->r1).
 *     So the diagnosis "the remaining difference is register allocation" was
 *     WRONG; the sole cause was the shared local.
 *
 * (2) THE BASE COPY `adds r0,r3,#0`.  26 -> 0 bytes of difference.
 *     The ROM copies the base into a SEPARATE register and uses that copy in
 *     two places:
 *         800a9f0  adds r0,r3,#0      <- the copy
 *         800a9f2  ldr  r1,[r0,#8]    <- from the copy (flag branch)
 *         800aa16  ldr  r0,[r0,#4]    <- from the copy (midpoint branch)
 *         800a9fa  ldr  r0,[r3,#4]    <- from the ORIGINAL base (fall-through)
 *     What produces the copy is NOT `probe = block;` (see below) but WRITING
 *     THE SYMBOL TWICE: `probe = &gRam02011030;` in the flag branch and
 *     `block = &gRam02011030;` in the fall-through branch.  Because the two
 *     references are in DIFFERENT CSE blocks, CSE does not reduce them to a
 *     single register; the common subexpression then moves the pool load ahead
 *     of the branch and leaves a reg-reg copy in the flag branch.  The result
 *     is exactly the ROM's shape: the pool load at 0x800a9ea (before the
 *     branch, serving the fall-through) and the copy at 0x800a9f0 (serving the
 *     flag and midpoint branches).
 *
 * ============== FORMS TRIED AND ELIMINATED (do not repeat) ==============
 *
 * THE COPY CLASS -- all of these DESTROYED the copy, and the output did not
 * change (26 differences):
 *   - `probe = block;`                       (tried in three separate sessions)
 *   - `probe = block + 0;`
 *   - `probe = (CoordBlock *)((char *)block + 0);`
 *   - `probe = &gRam02011030;` BUT with `block` still in the same EBB
 *     (H_twoRefs_order: `block = &gRam02011030;` moved BEFORE the branch).
 *     CRITICAL: the two references MUST be in separate CSE blocks; if both are
 *     in the entry block, CSE reduces them to a single register.
 *   - A pre-assignment `probe = 0;` (dead code, DCE removes it).
 *   - Permutations of DECLARATION order (block/probe/flag first, probe last,
 *     block last -> ALL FIVE give 26 differences).  Pseudo numbers DO NOT
 *     change the direction of CSE's canonicalisation.
 *
 * THE CFG CLASS -- all give 26 differences (so changing the CFG alone is not
 * enough; the same RTL comes out as with base26):
 *   - `if (flag == 0 || (second = probe->unk08) == 0) { fallthrough; return; }`
 *   - The same with the comma operator: `(probe = block, second = ...)`
 *   - `if (... && ...) goto midpoint;`
 *   - The double goto `if (second == 0) goto simple; goto midpoint;`
 *
 * THE MERGE CLASS -- KEPT the copy alive but 2 bytes TOO EXPENSIVE (81
 * differences, 92 bytes):
 *   - Moving the second test OUT of the if body with `second = 0;`.  A
 *     multiply-referenced code_label lands in between, the CSE block ends
 *     there, and the copy survives.  But `movs r1,#0` costs an extra 2 bytes
 *     plus 2 bytes of padding for the pool alignment.  The right mechanism,
 *     the wrong price.
 *   - Merging `second` with a sentinel such as `&gRam02011030`: 96 bytes, 86
 *     differences.  Much worse.
 *
 * A DIAGNOSTIC TOOL (in case someone else hits the same wall): agbcc ACCEPTS
 * the `-da` flag.  `old_agbcc -mthumb-interwork -O2 -fhex-asm -da -o x.s x.i`
 * leaves an RTL dump for every pass (x.i.rtl, x.i.jump, x.i.cse, x.i.gcse,
 * x.i.loop, x.i.cse2, x.i.flow, x.i.combine, x.i.regmove, x.i.lreg, x.i.greg).
 * That is how we MEASURED where the copy goes: `probe = block` stands as insn
 * 9 in the rtl, CSE turns both of its uses into `block` (insn 24 and insn 61),
 * and the `flow` pass deletes the now-dead copy.  That it is also deleted
 * under `-fno-cse-follow-jumps` showed the cause is NOT jump following but
 * canonicalisation within the same block.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/update_focus.c
 */

#include "gba_types.h"
#include "game_state.h"

typedef struct Vec2 {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
} Vec2;

typedef struct Target {
    u8    pad00[24];
    Vec2 *pos;                  /* +0x18 */
} Target;

typedef struct CoordBlock {
    u32 unk00;                  /* +0  */
    u32 unk04;                  /* +4  */
    u32 unk08;                  /* +8  */
    s32 second;                 /* +12 */
    s32 first;                  /* +16 */
    u8  pad14[28];
    u32 a;                      /* +0x30 */
    u32 b;                      /* +0x34 */
    u32 c;                      /* +0x38 */
    u8  pad3C[12];
    u32 unk48;                  /* +72 */
    u8  pad4C[37];
    u8  byte71;                 /* +0x71 */
} CoordBlock;

extern CoordBlock gRam02011030;

extern Vec2       gFocusPoint;

/* 0x0800A9E4 */
void UpdateFocusPoint(void)
{
    CoordBlock *block;
    CoordBlock *probe;
    Target *first;
    Target *second;
    Target *mfirst;
    Vec2 *out;
    Vec2 *mout;
    u32 flag;

    /* ORDER: the ROM READS the gGameState flag first.  The block base is
       written SEPARATELY in the flag branch and in the fall-through branch
       (measurement 2 above): because the two references are in different CSE
       blocks, the ROM's `ldr r3,=block` + `adds r0,r3,#0` pair comes out. */
    flag = gGameState.flag;
    if (flag != 0) {
        probe = &gRam02011030;
        second = (Target *)probe->unk08;
        if (second != 0)
            goto midpoint;
    }

    /* The fall-through branch reads from the original base (ROM:
       ldr r0,[r3,#4]).  Rule 45 -- this branch's locals must be SEPARATE from
       the midpoint branch's, otherwise the final store is merged by
       cross-jumping. */
    block = &gRam02011030;
    out = &gFocusPoint;
    first = (Target *)block->unk04;
    out->x = first->pos->x;
    out->y = first->pos->y;
    return;

midpoint:
    /* Orta nokta dali: +0x04 KOPYADAN okunuyor (ROM: ldr r0,[r0,#4]).
       Rule 49 -- the ROM keeps this rare body at the END of the function. */
    mout = &gFocusPoint;
    mfirst = (Target *)probe->unk04;
    mout->x = (mfirst->pos->x + second->pos->x) >> 1;
    mout->y = (mfirst->pos->y + second->pos->y) >> 1;
}
