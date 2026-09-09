/* SpawnFollowupEntry — 0x080291B8-0x0802922F (120 bytes)
 *
 * Takes a gEntriesA entry as a template, builds a new kind-34 entry from it,
 * then refreshes two global flags.  The phase choice depends on the entry's
 * kind/phase:
 *
 *   kind (+0x64) == 76  OR  phase (+0x90) == 46
 *       -> CreateEntryFromTemplate(e, 1, 0, 34, 47, e->owner)
 *          FUN_08035058(GetActiveSlotValue(), 461)
 *   otherwise
 *       -> CreateEntryFromTemplate(e, 1, 0, 34, 7, e->owner)
 *          FUN_08035058(GetActiveSlotValue(), 258)
 *
 * In both cases it ends with gRam020245A0 = 1 and gRam02024344 = 0.
 *
 * SIBLING: this body is EXACTLY the same as the second work block inside
 * src/world/entries_b5.c (StepEntryPhase, 0x08025518).  The struct layout,
 * the call signatures and the RAM symbols were taken from there; since that
 * one is already byte-matching, the correctness of the constants/arguments is
 * proven a second time.
 *
 * DETAILS MEASURED FROM THE ROM
 * -----------------------------
 *  1. Rule 35 -- RETURN TYPE is void.  Epilogue `add sp,#8; pop {r0}; bx r0`:
 *     the return address is taken into r0, so r0 is NOT live.  If it returned
 *     a value (as in the sibling StepEntryPhase) it would be
 *     `pop {r1}; bx r1`.
 *
 *  2. PROLOGUE `push {lr}` -- NO callee-saved register at all.  `e` is only
 *     taken into r1 with `adds r1,r0,#0` and is consumed BEFORE the last call
 *     (while CreateEntryFromTemplate's arguments are being set up).  Because
 *     the following two calls do not touch e, agbcc can keep it in the
 *     caller-saved r1.  This says that in the source `e` must NOT be COPIED
 *     into a local that crosses a call boundary -- the parameter was used
 *     directly.  `sub sp,#8` is for the two stack arguments (the 5th and 6th).
 *
 *  3. The FIELD ACCESSES come out as `adds rX,#100` / `adds rX,#144` plus an
 *     offset-0 load; this is Thumb's mandatory form (ldrb imm5 <= 31,
 *     ldr imm5*4 <= 124), so here there is no choice between rule 2's two
 *     spellings -- plain `e->kind` / `e->phase` is correct.
 *
 *  4. `e->owner` (+0x01) is read with `ldrb` and written to the stack with
 *     `str`: a narrow field in a wide (u32) parameter slot.  The last argument
 *     of the CreateEntryFromTemplate signature in the sibling is `u8 owner`;
 *     that same signature produces exactly this ldrb/str pair here as well.
 *
 *  5. THE TWO NOTIFICATION CONSTANTS ARE BUILT BY DIFFERENT ROUTES, and this
 *     happens ON ITS OWN, no lever is needed in the source:
 *       461 = 0x1CD -> cannot be built as imm8<<n, it is loaded from the pool.
 *              The pool is at 0x080291F0, that is, IMMEDIATELY AFTER the first
 *              branch's `b` instruction -- agbcc dumps the pool at the first
 *              unreachable point.
 *       258 = 0x81<<1 -> `movs r1,#129; lsls r1,#1`.
 *     In the source both are plain `#define` constants; rule 44 (moving the
 *     constant into a local) is NOT NEEDED, because here it is an argument,
 *     not a comparison.
 *
 *  6. The CONDITION ORDER is the same as the source order: first `cmp #76`
 *     (kind), then `cmp #46` (phase).  Because the two terms of the `||`
 *     chain are NOT adjacent equalities (different fields), rule 44's range
 *     folding is not triggered; the `c47` local that the 46/47 pair in the
 *     sibling needs is unnecessary here.
 *
 *  7. The `if/else` was KEPT, rule 29 is not needed: in the ROM the two call
 *     blocks stand separately and the common tail (two stores + epilogue)
 *     meets at 0x8029214 -- that is, agbcc's cross-jumping already does what
 *     the ROM wants here.  The first branch jumps to the tail with
 *     `b 0x8029214`, the second branch FALLS THROUGH into the tail (a layout
 *     consistent with rule 49).
 *
 * ROUTES TRIED AND REJECTED
 * -------------------------
 *  (NONE -- IT MATCHED ON THE FIRST ATTEMPT, not a single variant was tried.
 *   Trusting the same block of the sibling entries_b5.c was enough: the
 *   struct layout, the signatures, the constants and the if/else form were
 *   taken from there unchanged.  New attempts must be added HERE, the
 *   existing notes must not be deleted.)
 *
 * MATCH: 120/120 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_a9.c
 */

#include "gba_types.h"

#define KIND_34    34           /* kind of the new entry, +0x64 */
#define KIND_76    76           /* threshold for the template entry's kind */

#define PHASE_7     7           /* phase passed to CreateEntryFromTemplate */
#define PHASE_46   46           /* threshold for the template entry's phase */
#define PHASE_47   47           /* phase passed to CreateEntryFromTemplate */

#define NOTIFY_A  461           /* second argument of FUN_08035058 */
#define NOTIFY_B  258

/* The 12-byte triple at +0x4C; the same object as `Triple` in
 * src/world/entries_b5.c and `Pack12` in src/world/submit_pack.c.  This
 * function does not touch its contents, it is here only to complete the
 * layout. */
typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

/* The same layout as the sibling files (entries_a5.c, entries_a6.c,
 * entries_b1.c, entries_b5.c); 148 = 0x94 in total. */
typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     owner;               /* +0x01, carried from the template to the new entry */
    u8     pad02[2];
    u8     sub[38];             /* +0x04 */
    u8     pad2a[34];
    Triple payload;             /* +0x4C */
    u8     pad58[12];
    u8     kind;                /* +0x64 */
    u8     pad65[31];
    u32    unk84;               /* +0x84 */
    u8     pad88[8];
    u32    phase;               /* +0x90, stride 148 */
} Entry;

/* Their addresses are recorded in data/ram_map.csv (0x020245A0 and
 * 0x02024344); the build layer generates the `.equ` declarations itself. */
extern u8  gRam020245A0;
extern u16 gRam02024344;

extern u32  GetActiveSlotValue(void);
extern void FUN_08035058(u32 slot, u32 id);
extern u32  CreateEntryFromTemplate(Entry *src, u32 unused1, u32 unused2,
                                    u8 kind, u32 phase, u8 owner);

/* 0x080291B8 */
void SpawnFollowupEntry(Entry *e)
{
    if (e->kind == KIND_76 || e->phase == PHASE_46) {
        CreateEntryFromTemplate(e, 1, 0, KIND_34, PHASE_47, e->owner);
        FUN_08035058(GetActiveSlotValue(), NOTIFY_A);
    } else {
        CreateEntryFromTemplate(e, 1, 0, KIND_34, PHASE_7, e->owner);
        FUN_08035058(GetActiveSlotValue(), NOTIFY_B);
    }

    gRam020245A0 = 1;
    gRam02024344 = 0;
}
