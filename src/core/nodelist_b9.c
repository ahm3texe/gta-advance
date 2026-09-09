/* Rebuild the session from the pending area name -- 0x08053B44, 516 bytes.
 *
 * The "pending name" field (+0x08) of the gUnk02010C60 session block says
 * which record group is to be loaded; the return stack (+0x10) is peeled back
 * by one level, and the area context is then rebuilt from scratch:
 *
 *   1. Two 3-entry name tables are COPIED onto the stack (ldmia/stmia pairs):
 *        gEmptyAreaNames   0x083E3004 -> {"empty1","empty2","empty3"}
 *        gSpecialAreaNames 0x083E3010 -> {"special1","special2","special3"}
 *      Both are locals; the ROM writes them at sp+0 and sp+12 and keeps the
 *      address of sp+12 in r9.
 *   2. The pending name is first searched in the SECONDARY id array of the
 *      RECORD GROUPS (gAreaBank.groups[i].idsB). On a hit the group index,
 *      otherwise ID_NONE.  The comparator FUN_0806dd18 is strcmp (the body at
 *      0x0806DD18 is the classic word-aligned strcmp).
 *   3. If the stack depth is nonzero and the selected group is the TOP of the
 *      stack, the stack is cleared; if it is still non-empty, the name is
 *      taken again from the stack (from the empty/special table) and the index
 *      becomes depth-1.
 *   4. This time the name is searched in the FULL name table (gAreaBank.names,
 *      228 entries).
 *   5. gAreaNames[nameIdx].flags bit 3, or the session's +0x1C flag ->
 *      gRam020004A0 = 1; if neither, 0.
 *   6. The record, the free lists and the area context are rebuilt.
 *
 * STRUCTURE HINTS READ FROM THE ROM:
 *   - gAreaBank (0x08D49C00) is seen here through four of its fields:
 *       +0x08 nameCount = 228, +0x14 groupCount = 3,
 *       +0x20 names = 0x08D482E0, +0x2C groups = 0x08D49BD0.
 *     +0x24 (entries) was measured in src/core/nodelist_d5.c and +0x08/+0x20
 *     in src/core/nodelist_b2.c; +0x14 was added here.
 *   - AreaGroup (16 bytes) is the wider form of the one in
 *     src/core/nodelist_d5.c: +0x00 name ("gta_script_island1..3"),
 *     +0x04 primary count, +0x05 SECONDARY count, +0x08 primary id array,
 *     +0x0C SECONDARY id array.  This function uses ONLY the secondary pair
 *     (+0x05 / +0x0C) -- not the sibling file's +0x04/+0x08.
 *   - AreaName is 28 bytes: +0x00 the name pointer (same as nodelist_b2.c),
 *     +0x1A the flag byte.  Of the 228 entries in the ROM, 28 have bit 3 set.
 *
 * SIX MEASURED DETAILS (each was tried on its own; all six are decisive):
 *
 * 1) THE TWO NAME TABLES MUST BE SEPARATE SYMBOLS; a bare address will not do.
 *    Written as `*(const NameTable *)0x083E3010`, CSE sees that after the
 *    first `ldmia r0!` r0 is ALREADY 0x083E3004+12 = 0x083E3010 and drops the
 *    second pool load entirely: 508 bytes (2 bytes of instruction + 4 bytes of
 *    pool + 2 bytes of alignment = 8 short).
 *    With two SEPARATE extern symbols, CSE cannot reason about symbol
 *    arithmetic and the ROM's two `ldr r0,pool` instructions come back.  This
 *    function CANNOT MATCH with a bare address.
 *
 * 2) gAreaNames MUST BE DECLARED AS AN ARRAY, not as a pointer constant.
 *    `((const AreaName *)ADDRESS)[nameIdx].flags` puts the pool load AFTER the
 *    scaling:
 *        lsls/subs/lsls, ldr rB,pool, adds
 *    while the ROM loads the base first:
 *        ldr r2,pool, lsls/subs/lsls, adds r0,r0,r2
 *    `extern const AreaName gAreaNames[];` (a real ARRAY_REF) gives the ROM's
 *    order.  It also moves nameIdx's register from r0 to the ROM's r1 -- a
 *    single change that closes two differences at once.
 *
 * 3) THE SEARCH RESULT AND THE STACK INDEX ARE TWO SEPARATE LOCALS, AND THE
 *    DIRECTION OF THE COPY MATTERS.
 *    The ROM emits `adds r1,r7,#0`: the searched value is born in r7 and
 *    COPIED to r1; r7 is then OVERWRITTEN by `slot = depth - 1`, while r1
 *    carries the old value and is used in the `sel + 1 != depth` test at
 *    0x08053C44.  So the search result must be written into `slot` and then
 *    `sel = slot;`.  The reverse form (searching into `sel` and writing
 *    `slot = sel;`) inverts the copy and swaps the two registers.  The copy is
 *    NOT eliminated here because the FINAL values of the two variables differ
 *    -- the rule "copy-based splitting is always eliminated" only holds while
 *    both stay the same.
 *
 * 4) The result of the SECOND search uses an `int raw` + `u16` pair (the rule
 *    15/35 family, the same pattern as src/core/nodelist_b2.c).  Writing
 *    DIRECTLY into a `u16 nameIdx` emits the `lsls/lsrs` narrowing in every
 *    branch; the ROM narrows once, where the branches MERGE.
 *    `rawName` (int) is used in the branches and `nameIdx = rawName;` at the
 *    merge point.  The first search has NO such narrowing -- a single `int`
 *    is enough there.
 *
 * 5) A SEPARATE POINTER LOCAL FOR THE CANDIDATE IN THE FIRST LOOP (rule 4 /
 *    d5 point 4).
 *    Written plainly, `gAreaBank.names[...].name` puts `ldr rX,[r6,#32]`
 *    BEFORE the ldrh; the ROM reads the id first, computes id*28, loads the
 *    `names` member AFTERWARDS and leaves the sum in the BASE register.
 *    `const AreaName *cand = &gAreaBank.names[...];` gives that order.
 *    In the SECOND loop it is the other way round: there the ROM already
 *    loads the member first, so a separate local is NOT needed -- this is
 *    measured, not memorised from sibling files.
 *
 * 6) THE TWO ZERO ASSIGNMENTS ARE CHAINED: `gRam02026F34 = gRam020272C8 = 0;`.
 *    The ROM loads both addresses BEFORE the stores, then stores them in
 *    reverse order (`ldr r1,=..2026F34 / ldr r0,=..20272C8 / str [r0] /
 *    str [r1]`).  That is exactly the expansion order of a chained
 *    assignment: the TARGET address of the outer assignment is resolved
 *    first, then the inner assignment runs (address + value + store), and the
 *    outer store happens last.  Writing two separate statements puts each
 *    store's own pool load in front of it.
 *
 * 7) THE RECORD POINTER MUST BE TAKEN INTO A SEPARATE LOCAL.
 *    Written as `gRam020357E0 = GetRecord(slot)->f20;`, GCC resolves the
 *    LEFT-hand address first; the symbol load rises ABOVE the `bl GetRecord`
 *    and holds callee-saved r4.  The ROM loads the address AFTER the call
 *    (r1).  `rec = GetRecord(slot); gRam020357E0 = rec->f20;` forces the
 *    right-hand side to be evaluated first and gives the ROM's order.
 *
 * THE MIDDLE BLOCK (0x08053C06-0x08053C5A) MATCHED ON THE FIRST WRITING:
 *   `sel + 1 == depth` -> if we are at the top of the stack, clear it; if the
 *   stack is still non-empty, `slot = depth - 1` and the name comes from the
 *   empty table; then, if the special flag is set, the name comes from the
 *   special table; if the flag is clear and the stack is still non-empty, from
 *   the empty table again.  The last two branches are joined into a shared
 *   tail in the ROM (ldr + mov r8); writing them as two separate assignments
 *   in the source is enough, the compiler merges the tail itself.
 *
 * OTHER RULES APPLIED:
 *   - Rule 1: gAreaBank / gAreaNames / the session block are extern symbols.
 *   - Rule 9/31: all counters are `int`; the ROM uses `blt`/`bge` (signed).
 *   - Rule 35: `pop {r0}; bx r0` -> a void return type.
 *   - Rule 45: each loop gets its own counter (i, j, k); a shared counter ties
 *     the first loop to the second one's allocation.
 *   - Rule 49: the rare branches (the ID_NONE assignments) were left in their
 *     own blocks with `goto found`, as in the ROM.
 *
 * TRIED AND ELIMINATED (do not retry):
 *   - A bare address via `#define` (for the three ROM tables): point 1, 508
 *     bytes.
 *   - `entry = &gAreaNames[nameIdx];` as a separate pointer local: it does NOT
 *     change the wrong order in point 2; what decides it is the declaration
 *     being an ARRAY.
 *   - Assigning directly into a `u16 nameIdx`: point 4.
 *   - Searching into `sel` and writing `slot = sel;`: point 3, swaps two
 *     registers.
 *   - Writing the two zero assignments as separate statements: point 6.
 *   - `gRam020357E0 = GetRecord(slot)->f20;` on one line: point 7.
 *
 * RECORDS REQUIRED IN data/ram_map.csv (this file DOES NOT COMPILE without
 * them):
 *   0x083E3004,12,gEmptyAreaNames     ROM: 3 x char*  {"empty1".."empty3"}
 *   0x083E3010,12,gSpecialAreaNames   ROM: 3 x char*  {"special1".."special3"}
 *   0x08D482E0,6384,gAreaNames        ROM: 228 x 28-byte AreaName; the same
 *                                     value as gAreaBank.names (+0x20)
 *   0x020357E0,4,gRam020357E0         GetRecord(slot)->f20 is written here
 *   0x02035760,0,gRam02035760         the FIRST argument of FUN_08053834
 *                                     (AreaCtx); size UNKNOWN, only the base
 *                                     was measured
 *
 * VERIFICATION (a measurement made before the records were added):
 *   The five symbols were replaced with five symbols of the same type ALREADY
 *   recorded in ram_map (gRom08BD3448 / gRom08CA61C0 / gRom08852A2C /
 *   gRam02001450 / gRam020110C0).  The result was 516/516 bytes with not a
 *   SINGLE instruction deviating; the only difference stayed in the five pool
 *   CONSTANTS that had been changed.  With the real addresses recorded the
 *   match is exact; the address value does not affect code generation because
 *   symbol arithmetic is not visible at compile time.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_b9.c
 */

#include "gba_types.h"

#define ID_NONE  0x7FFF

/* One name per stack level; two 12-byte constant tables in the ROM. */
typedef struct NameTable {
    const char *entry[3];       /* +0x00 */
} NameTable;

/* A 28-byte name entry; the same as AreaName in src/core/nodelist_b2.c, with
 * the +0x1A flag byte added. */
typedef struct AreaName {
    const char *name;           /* +0x00 */
    u8          pad04[22];      /* +0x04 */
    u8          flags;          /* +0x1A bit 3 -> gRam020004A0 = 1 */
    u8          pad1B;          /* +0x1B */
} AreaName;

/* A 16-byte record group; AreaGroup from src/core/nodelist_d5.c with the
 * secondary count/array pair added. */
typedef struct AreaGroup {
    const char *name;           /* +0x00 */
    u8          countA;         /* +0x04 */
    u8          countB;         /* +0x05 */
    u8          pad06[2];       /* +0x06 */
    u16        *idsA;           /* +0x08 */
    u16        *idsB;           /* +0x0C */
} AreaGroup;

typedef struct AreaBank {
    u8         pad00[8];        /* +0x00 */
    s32        nameCount;       /* +0x08 */
    u8         pad0C[8];        /* +0x0C */
    s32        groupCount;      /* +0x14 */
    u8         pad18[8];        /* +0x18 */
    AreaName  *names;           /* +0x20 */
    u8         pad24[8];        /* +0x24 (entries in nodelist_d5.c) */
    AreaGroup *groups;          /* +0x2C */
} AreaBank;

/* The 32-byte session block at 0x02010C60.  Its +0x1A byte is read as
 * gUnk02010C60[26] in other translation units; it is not used here, so it was
 * left in the padding. */
typedef struct Session {
    u8          pad00[8];       /* +0x00 */
    const char *pending;        /* +0x08 pending area name */
    u8          pad0C[4];       /* +0x0C */
    u8          depth;          /* +0x10 depth of the return stack */
    u8          pad11[11];      /* +0x11 */
    u16         special;        /* +0x1C switch-to-special-table flag */
    u8          pad1E[2];       /* +0x1E */
} Session;

/* The 60-byte record returned by GetRecord (the 0x08CAC248 table). */
typedef struct Record {
    u8  pad00[0x20];            /* +0x00 */
    u32 f20;                    /* +0x20 */
    u8  pad24[0x18];            /* +0x24 */
} Record;

/* The area context set up by FUN_08053834; its internal layout is NOT
 * expanded here because it is not used in this file (AreaCtx in
 * src/core/nodelist_b2.c). */
typedef struct AreaCtx AreaCtx;

extern const NameTable gEmptyAreaNames;     /* 0x083E3004 */
extern const NameTable gSpecialAreaNames;   /* 0x083E3010 */
extern const AreaName  gAreaNames[];        /* 0x08D482E0 */
extern AreaBank        gAreaBank;
extern Session         gUnk02010C60;
extern AreaCtx         gRam02035760;
extern u32             gRam020004A0;
extern u32             gRam020357E0;
extern u32             gRam02026F34;
extern u32             gRam020272C8;
extern u32             gRam02025800;

extern s32     FUN_0806dd18(const char *a, const char *b);   /* strcmp */
extern Record *GetRecord(s32 index);
extern void    BuildNodeFreeLists(void);
extern void    ResetRuntimeGlobals(void);
extern u32     FUN_08053834(AreaCtx *ctx, AreaGroup *group, const char *name);
extern void    FUN_08051154(s32 index);

/* 0x08053B44 */
void LoadAreaByName(void)
{
    NameTable   empty;
    NameTable   special;
    AreaGroup  *group;
    Record     *rec;
    const char *name;
    int         sel;            /* raw search result, before the stack is peeled */
    int         slot;           /* group to load: sel, or depth-1 */
    int         rawName;        /* rule 15 family: narrowing happens at the merge */
    u16         nameIdx;
    int         i;
    int         j;
    int         k;              /* rule 45: a separate counter per loop */

    empty = gEmptyAreaNames;
    special = gSpecialAreaNames;

    /* 1. Which record group's secondary list holds the pending name? */
    name = gUnk02010C60.pending;
    if (name == 0) {
        slot = ID_NONE;
        goto found;
    }
    for (i = 0; i < gAreaBank.groupCount; i++) {
        for (j = 0; j < gAreaBank.groups[i].countB; j++) {
            /* A separate pointer local: point 5 in the header above. */
            const AreaName *cand =
                &gAreaBank.names[gAreaBank.groups[i].idsB[j]];

            if (FUN_0806dd18(cand->name, name) == 0) {
                slot = i;
                goto found;
            }
        }
    }
    slot = ID_NONE;
found:
    sel = slot;                 /* the DIRECTION of the copy matters: point 3 above */

    /* 2. The return stack: if we are at the top, empty it; otherwise step
     *    back one level from the top and take the name from the stack
     *    table. */
    if (gUnk02010C60.depth != 0) {
        if (sel + 1 == gUnk02010C60.depth)
            gUnk02010C60.depth = 0;
        if (gUnk02010C60.depth != 0) {
            slot = gUnk02010C60.depth - 1;
            name = empty.entry[slot];
        }
    }
    if (gUnk02010C60.special != 0) {
        name = special.entry[slot];
    } else if (sel + 1 != gUnk02010C60.depth) {
        if (gUnk02010C60.depth != 0)
            name = empty.entry[slot];
    }
    gUnk02010C60.depth = 0;

    /* 3. The same name, this time in the full name table. */
    if (name == 0) {
        rawName = ID_NONE;
        goto found2;
    }
    for (k = 0; k < gAreaBank.nameCount; k++) {
        if (FUN_0806dd18(gAreaBank.names[k].name, name) == 0) {
            rawName = k;
            goto found2;
        }
    }
    rawName = ID_NONE;
found2:
    nameIdx = rawName;          /* the narrowing happens exactly here: point 4 */

    if ((gAreaNames[nameIdx].flags & 8) != 0 || gUnk02010C60.special != 0)
        gRam020004A0 = 1;
    else
        gRam020004A0 = 0;

    /* 4. Rebuild the record and the area context. */
    group = &gAreaBank.groups[slot];
    rec = GetRecord(slot);      /* a separate local: point 7 above */
    gRam020357E0 = rec->f20;
    gRam02026F34 = gRam020272C8 = 0;    /* chained: see note 6 */
    BuildNodeFreeLists();
    ResetRuntimeGlobals();
    FUN_08053834(&gRam02035760, group, name);
    FUN_08051154(slot);
    gRam02025800 = 0;
}
