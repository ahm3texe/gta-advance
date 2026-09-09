/* StepEntryPhase — 0x08025518-0x080256E1 (458 bytes)
 *
 * Processes a gEntriesA entry according to its kind (+0x64) and phase (+0x90).
 * It does three things:
 *   1. For kinds 101 / 51 / 76 / 34, it looks at the phase, picks one of two
 *      constants (0x000E0000 or 0x00300000) and passes it to the
 *      FUN_08023df0 + FUN_08060db4 pair together with the 12-byte triple at
 *      +0x4C.  The fourth argument gets gRom08CA61C0[e->owner] << 16.
 *   2. If the kind is 51/76 or the phase is 22/46, it passes the same entry as
 *      a template and builds a new entry of kind 34 with
 *      CreateEntryFromTemplate (0x08025424, src/world/entries_b1.c); either
 *      phase 47 + notification 461, or phase 7 + notification 258.  It then
 *      sets gRam020245A0 = 1 and gRam02024344 = 0.
 *   3. If the phase is 12 / 38 / 39 it returns 1 and leaves the entry alive;
 *      otherwise it releases the sub-object with ReleaseObject(e->sub), clears
 *      +0x00 and returns 0.
 *
 * SIGNATURE: a single parameter (r0); the epilogue `pop {r1}; bx r1` takes the
 * return address into r1, so r0 is live and the function RETURNS A VALUE (the
 * inverse of rule 35).
 *
 * THE STRUCTS and CALL SIGNATURES were taken from sibling files: the Entry
 * layout is the same as in src/world/entries_b1.c and entries_a6.c (148
 * bytes); the `Triple` at +0x4C is the same object as `Pack12` in
 * src/world/submit_pack.c (FUN_08060db4 is called there with the same
 * (Triple *, u32) signature).
 *
 * ---------------------------------------------------------------------
 * DETAILS MEASURED FROM THE ROM (all verified by diff)
 *
 *  1. THE OUTER SWITCH really is a `switch`.  At 0x8025528 there is a
 *     `cmp #76 / beq` immediately followed by `cmp #76 / bgt`: that is agbcc's
 *     pattern for a node that is both a case value and a split point (the tree
 *     side of rule 46).  The `bgt` is SIGNED, because the `ldrb` promotes to
 *     int.
 *     The case set {34,51,76,101} is sparse, so no jump table comes out.
 *     The case bodies are emitted IN SOURCE ORDER; the ROM's order is
 *     101, 51/76, 34 -- and that is the order used in this file.
 *
 *  2. THE INNER SWITCH (`case 34`) is on the phase: `cmp #28 / beq`,
 *     `cmp #28 / bhi` -> an UNSIGNED branch, so `phase` is u32 (rule 31).
 *     Case 28 has an empty body.  The SOURCE ORDER of the inner cases must be
 *     28, 46, 22: the case left last has its `b` deleted and becomes a
 *     fall-through, which takes that block OUT of the cross-jumping candidate
 *     set and leaves it as a separate physical copy.  In the ROM the copy left
 *     separate is case 22's (0x8025604), which is why 22 was written last.
 *     Moving 22 to the front leaves case 46's separate instead and shifts the
 *     whole block layout.
 *
 *  3. RULE 44 WAS DECISIVE HERE.  agbcc's `fold` turns the form
 *     `if (e->phase == 46 || e->phase == 47)` into a RANGE TEST:
 *     `subs r0,#46 / cmp r0,#1 / bhi`.  The ROM uses two separate `cmp #46` /
 *     `cmp #47`.  Taking 47 into a local (`c47 = 47;`) dodges the folding, and
 *     the constant is still emitted as an immediate.
 *     The side effect is far larger: because of the range test, case 101's body
 *     was being merged with the others by CROSS-JUMPING; once separated, the
 *     ROM's THREE distinct call blocks appear.  A single line: 414 -> 458
 *     bytes.
 *     (The same folding does NOT happen in the 51/76 branch, because
 *     `e->kind == 76` stands at the head of the `||` chain and fold cannot see
 *     two adjacent equalities in the binary tree -- which is why c47 is not
 *     needed there.)
 *
 *  4. RULE 45 -- SEPARATE LOCALS PER BRANCH.  Each of the four call branches
 *     uses its own `p / w / tbl` triple.  Shared locals both merge the blocks
 *     and INVERT THE ALLOCATION: a shared `w` enters the global allocator,
 *     queues after `e` and takes r6, leaving `e` in r5 -- the INVERSE of the
 *     ROM.  Branch-local variables fit in a single block in the
 *     straight-line case bodies, so they drop to the LOCAL allocator, claim r5
 *     early and push `e` down to r6.
 *     Measured (all 458 bytes; only the difference count varies):
 *         all separate          -> 0 differences  (MATCH)
 *         `tbl` shared          -> 6 differences
 *         `p`   shared          -> 28 differences
 *         `w`   shared          -> 39 differences
 *         only 101 separate     -> 85 differences (e/r5, w/r6 inverted)
 *         none separate         -> 402 bytes (three copies collapse to two)
 *
 *  5. ARGUMENT ORDER: the ROM builds `gRom08CA61C0[owner] << 16` first, then
 *     &e->payload, and e->unk84 last.  That only comes out if the table read is
 *     a SEPARATE STATEMENT placed BEFORE `p = &e->payload;`.  Written on one
 *     line as
 *     `FUN_08023df0(&e->payload, w, e->unk84, TABLE[owner] << 16)`, the order
 *     becomes right-to-left and payload/unk84 swap.
 *
 *  6. RULE 1 -- THE TABLE MUST BE AN EXTERN SYMBOL.  A constant cast
 *     (`((u32 *)0x08CA61C0)[i]`) makes agbcc compute the index FIRST and load
 *     the pool constant AFTERWARDS:
 *         ldrb / lsl / ldr =base / add
 *     while the ROM loads the base FIRST:
 *         ldr =base / ldrb / lsl / add
 *     Verified with an isolated experiment (four forms: constant cast, an
 *     intermediate pointer local, integer arithmetic, and an array-pointer
 *     cast -- ALL FOUR gave the wrong order).  Only
 *     `extern u32 gRom08CA61C0[];` gives the right one.  Nine instructions
 *     across three blocks; that accounted for the whole final 20-byte gap.
 *
 *  7. RULE 49 -- THE RARE BODY GOES LAST.  In the tail the ROM keeps
 *     `return 1` in the flow and the cleanup block (`ReleaseObject` +
 *     `+0x00 = 0`) at the very end.  A plain `if (ok) return 1;` produces the
 *     INVERSE (the cleanup falls through and `return 1` is pushed to the end).
 *     Sending all three exits to the same label with `goto keep;` and placing
 *     that label BEFORE the cleanup block gives the ROM's order: 47 -> 20
 *     differences.
 *
 *  8. RULE 48 -- MATERIALISING THE CONDITION IN A VARIABLE.  At 0x80256B4
 *     there is the sequence
 *     `movs r4,#0 / cmp #39 / bne / movs r4,#1 / cmp r4,#0 / beq`; that is not
 *     direct branching but a flag variable.
 *     The final `strb r4,[r6,#0]` also uses the zero state of that variable
 *     (cse knows from the branch condition that r4 == 0), so a plain
 *     `e->active = 0;` is enough in the source.
 *
 * ---------------------------------------------------------------------
 * FORMS TRIED AND ELIMINATED (do not walk into the same wall)
 *
 *  - `if (e->phase == 46 || e->phase == 47)` (without the c47 local):
 *    414 bytes.  The range test both shortens it by 2 bytes and merges case
 *    101's call block with the others.  Reversing the order (`47 || 46`) does
 *    not save it either; fold still sees the adjacent range.
 *  - Writing the inner switch in the order `22, 28, 46`: 394 bytes instead of
 *    458.  The last case falling through is what decides it (point 2).
 *  - Taking the table base into a local (`tab = (u32 *)0x08CA61C0; tab[i]`):
 *    the copy is eliminated but the instruction order does not change (it
 *    stays at 47 differences).
 *    The same result came out of `u32 tab = 0x08CA61C0; *(u32 *)(tab + (i << 2))`
 *    and `#define TABLE (*(u32 (*)[])0x08CA61C0)`.  The constant load is always
 *    sunk to its use site.
 *  - `if (ok == 0) goto cleanup; return 1; cleanup: ...` in the tail: the block
 *    order DOES NOT CHANGE (47 differences).  All three exits have to go to the
 *    same label (point 7).
 *  - `FUN_08023df0(&e->payload, w, e->unk84, TABLE[owner] << 16)` on one line:
 *    the argument setup order is reversed (point 5).
 *
 * ---------------------------------------------------------------------
 * A NEW SYMBOL: 0x08CA61C0 (gRom08CA61C0) was NOT in data/ram_map.csv and
 * writing to that file was forbidden in that session, so the address was given
 * with a file-scoped `asm(".equ ...")`.  That was a TEMPORARY workaround; once
 * the line
 *     0x08CA61C0,0,gRom08CA61C0,decomp,provisional,
 *     "ROM: u32 table indexed by e->owner (+0x01); the value goes to
 *      FUN_08023df0 shifted <<16 (0x08025518)."
 * is added to ram_map, the `asm` line below should be deleted and only the
 * `extern` kept.  The same workaround was used in src/core/nodelist_c3.c (see
 * the file-header note there).  A constant cast CANNOT BE USED: under rule 1
 * the base is not held in a register and the instruction order shifts
 * (point 6).
 *
 * `make c-review` warns because of those two lines ("inline assembly" +
 * "bare address"); once the ram_map line is added and the `asm` deleted, the
 * check comes out CLEAN.  The generated assembly was measured to be IDENTICAL
 * without the `asm` line as well (the only difference is a repeated `.code 16`
 * directive, with no effect on the bytes), so the match survives that deletion.
 *
 * MATCH: 458/458 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_b5.c
 */

#include "gba_types.h"

#define KIND_34    34           /* +0x64 */
#define KIND_51    51
#define KIND_76    76
#define KIND_101  101

#define PHASE_7      7          /* the phase passed to CreateEntryFromTemplate */
#define PHASE_12    12
#define PHASE_22    22
#define PHASE_28    28
#define PHASE_38    38
#define PHASE_39    39
#define PHASE_46    46
#define PHASE_47    47

/* The second argument of FUN_08023df0 / FUN_08060db4; the ROM builds both as
 * imm8 << n (0xE0<<12 and 0xC0<<14). */
#define VALUE_A   0x000E0000
#define VALUE_B   0x00300000

#define NOTIFY_A   461          /* the second argument of FUN_08035058 */
#define NOTIFY_B   258

/* A ROM table; its address is recorded in data/ram_map.csv and the build layer
 * (tools/agbcc_build.py) generates the `.equ` declaration ITSELF -- which is
 * why there is NO inline assembly here.  The array declaration is mandatory:
 * other forms break the argument order (see the eliminated paths at the top of
 * the file). */
extern u32 gRom08CA61C0[];

/* The 12-byte triple at +0x4C; the same object as `Triple` in
 * src/world/entries_b1.c and `Pack12` in src/world/submit_pack.c. */
typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

/* The same layout as the sibling files (entries_a5.c, entries_a6.c,
 * entries_b1.c); the fields this function touches were expanded.  148 = 0x94
 * in total. */
typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     owner;               /* +0x01, also used as an index into the ROM table */
    u8     pad02[2];
    u8     sub[38];             /* +0x04, passed to ReleaseObject */
    u8     pad2a[34];
    Triple payload;             /* +0x4C */
    u8     pad58[12];
    u8     kind;                /* +0x64 */
    u8     pad65[31];
    u32    unk84;               /* +0x84 */
    u8     pad88[8];
    u32    phase;               /* +0x90, stride 148 */
} Entry;

extern u8  gRam020245A0;
extern u16 gRam02024344;

extern void FUN_08023df0(Triple *payload, u32 value, u32 arg2, u32 arg3);
extern void FUN_08060db4(Triple *payload, u32 value);
extern void ReleaseObject(u8 *sub);
extern u32  GetActiveSlotValue(void);
extern void FUN_08035058(u32 slot, u32 id);
extern u32  CreateEntryFromTemplate(Entry *src, u32 unused1, u32 unused2,
                                    u8 kind, u32 phase, u8 owner);

/* 0x08025518 */
u32 StepEntryPhase(Entry *e)
{
    /* Rule 45: every call branch has ITS OWN triple.  The declaration order
     * must not be changed; it decides the allocation order (see point 4 in the
     * file header). */
    Triple *p76;
    u32     w76;
    u32     tbl76;
    u32     ok;
    Triple *p101;
    u32     w101;
    u32     tbl101;
    u32     c47;                /* rule 44: the local that prevents the range
                                   test */
    Triple *p46;
    u32     w46;
    u32     tbl46;
    Triple *p22;
    u32     w22;
    u32     tbl22;

    switch (e->kind) {
    case KIND_101:
        c47 = PHASE_47;
        if (e->phase == PHASE_46 || e->phase == c47)
            w101 = VALUE_A;
        else
            w101 = VALUE_B;
        tbl101 = gRom08CA61C0[e->owner] << 16;
        p101 = &e->payload;
        FUN_08023df0(p101, w101, e->unk84, tbl101);
        FUN_08060db4(p101, w101);
        break;
    case KIND_51:
    case KIND_76:
        if (e->kind == KIND_76 || e->phase == PHASE_46 || e->phase == PHASE_47)
            w76 = VALUE_A;
        else
            w76 = VALUE_B;
        tbl76 = gRom08CA61C0[e->owner] << 16;
        p76 = &e->payload;
        FUN_08023df0(p76, w76, e->unk84, tbl76);
        FUN_08060db4(p76, w76);
        break;
    case KIND_34:
        switch (e->phase) {
        case PHASE_28:
            break;
        case PHASE_46:
            w46 = VALUE_A;
            tbl46 = gRom08CA61C0[e->owner] << 16;
            p46 = &e->payload;
            FUN_08023df0(p46, w46, e->unk84, tbl46);
            FUN_08060db4(p46, w46);
            break;
        case PHASE_22:          /* must stay last, see item 2 at the top */
            w22 = VALUE_B;
            tbl22 = gRom08CA61C0[e->owner] << 16;
            p22 = &e->payload;
            FUN_08023df0(p22, w22, e->unk84, tbl22);
            FUN_08060db4(p22, w22);
            break;
        }
        break;
    }

    if (e->kind == KIND_51 || e->kind == KIND_76
        || e->phase == PHASE_22 || e->phase == PHASE_46) {
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

    /* Rule 49: the cleanup block goes LAST; all three exits go to the same label. */
    if (e->phase == PHASE_12)
        goto keep;
    if (e->phase == PHASE_38)
        goto keep;
    ok = 0;                     /* rule 48: the condition is materialised in a
                                   variable */
    if (e->phase == PHASE_39)
        ok = 1;
    if (ok == 0)
        goto retire;
keep:
    return 1;
retire:
    ReleaseObject(e->sub);
    e->active = 0;
    return 0;
}
