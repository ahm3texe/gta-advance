/* Open a free slot in the gRam020245B0 table and set up a new entry — 0x08028B9C-0x08028C47
 *
 * Same family as its siblings src/world/entries_a5.c (0x08028A88) and
 * src/world/entries_b1.c (0x08025424): it searches a table with a 148-byte
 * stride for the first free slot whose +0x00 field is zero and, if one is
 * found, fills it in and sets up the sub-object (sub, +0x04). The differences:
 *   - the table is not gEntriesA but the SINGLE-ENTRY gRam020245B0
 *     (data/ram_map.csv:74),
 *   - the descriptor is resolved from the ROM root through CONSTANT indices:
 *       desc = gRom08BD3448.slots[64]->slots[0]->slots[0]
 *   - the slot's +0x68 and +0x4C fields come from the object supplied by the
 *     caller,
 *   - if no free slot is found it returns 0xFF, and on success THE SLOT INDEX.
 *
 * The flow:
 *   desc  = gRom08BD3448.slots[64]->slots[0]->slots[0]
 *   if there is no free slot, return 0xFF
 *   e->unk68   = h->unk18->unk0c
 *   e->payload = h->unk14->items[index]      (the 12-byte triple)
 *   FUN_08014FFC(e->sub, 7, &e->payload, &e->unk68)
 *   FUN_08013CFC(e->sub, desc, arg2)
 *   FUN_08014EE4(e->sub, desc->unk14)
 *   FUN_08015038(e->sub)
 *   e->active = 1;  return i;
 *
 * DETAILS MEASURED FROM THE ROM
 * -----------------------------
 * THE SIGNATURE COULD NOT BE VERIFIED from a call site: there is no single BL
 * to 0x08028B9C in the ROM, and the function's address (0x08028B9C /
 * 0x08028B9D) does not appear in any data word -- so the call is made
 * indirectly, from a table that has not been scanned. The arity was therefore
 * read from THE BODY alone: r3 is used, so there are at least four parameters;
 * r1 is overwritten at 0x8028BB2 with `movs r1,#128` without ever being read,
 * so THE SECOND ARGUMENT IS UNUSED.
 * That entries_b1.c also has two dead arguments shows this is normal in this
 * family.
 *
 * PARAMETER TYPES (rule 15): the third and fourth arguments are narrowed at
 * entry with an `lsls #24 / lsrs #24` pair -> UNSIGNED 8-bit.
 * The first argument is a raw pointer and the second is dead (left as u32).
 *
 * The inverse of rule 35: the epilogue `pop {r1}; bx r1` takes the return
 * address into r1 because r0 is live. The function RETURNS A VALUE. On the
 * success path `adds r0,r6,#0` (the loop counter), on the failure path
 * `movs r0,#255`. Because the counter is s32, the return type was left u32;
 * making it u8 risks adding a narrowing instruction to the return.
 *
 * THE LOOP FORM WAS READ FROM THE ROM, NOT COPIED FROM THE SIBLING:
 *   0x8028BD4  cmp r6,#0 / bgt   -> the loop condition `i < 1` (agbcc
 *                                   canonicalizes it to `<= 0`, the inverse
 *                                   direction of rule 44)
 *   0x8028BDE  cmp r6,#1 / beq   -> the post-loop check `i == 1`
 * So the bound is 1, which agrees exactly with the "1 x 148 entries" note in
 * ram_map. Even though there is only one iteration, agbcc still rotates the
 * loop: the entry test is eliminated, the first `ldrb` stays outside the body,
 * and the rest becomes a do/while.
 *
 * Rule 49: the failure body (`movs r0,#255`) sits at the END of the function,
 * after the literal pool. A plain `if (i == ENTRY_COUNT) return FAIL;` produces
 * that by itself; no label was needed.
 *
 * Rule 32: the 12 bytes at +0x4C are moved with an
 * `ldmia r1!,{r2,r3,r4}` / `stmia r0!,{...}` pair; that only comes out of a
 * struct assignment (Triple).
 * The `ldmia` clobbers r4 (the first argument), so the caller's object is dead
 * from that point on -- and it is not used after that in the source either.
 *
 * THE SHAPE OF THE SOURCE INDEX (rule 28): the address of the 12-byte element
 * is formed by computing index*12 FIRST with `lsls #1 / adds / lsls #2` and
 * adding it to the base, followed by `adds r1,#116`. That is the signature of
 * the array-index form (`p->items[i]`), not of pointer arithmetic that scales
 * each term separately.
 *
 * THE CONSTANT-INDEX ROM CHAIN: index 64 is 0x100 as a word offset, outside
 * Thumb's `ldr` immediate range, so `movs r1,#128 / lsls r1,#1 /
 * adds r0,r0,r1 / ldr r0,[r0]` comes out. Because the next two stages have
 * offset 0, they reduce to the pairs `ldr r0,[r0,#4] / ldr r0,[r0,#0]`.
 *
 * TRIED AND ELIMINATED (all measured in this file, with the form below matching)
 * ----------------------------------------------------------------------------
 * - Writing the post-loop check as `if (i > 0)`: 3/172 differences. agbcc
 *   pushes it into the same canonical form as the loop's own `cmp #0`/`bgt`
 *   test and merges them; the ROM has a SEPARATE `cmp #1`/`beq` at that point.
 *   The same family as rule 44: an equality test written with the bound value
 *   escapes the canonicalization. The same thing was solved with `i == 15` in
 *   entries_b1.c.
 * - Writing the loop condition as `i <= 0`: IT MATCHES, byte-for-byte the same
 *   code as `i < ENTRY_COUNT`. So the canonicalization really does happen in
 *   the loop test; the only place that distinguishes them is the POST-loop
 *   check. `<` was kept for readability.
 * - Making the return type `u8`: 176 bytes, 29 differences. agbcc adds an
 *   `lsls #24 / lsrs #24` narrowing on every `return` path; the ROM has none.
 *   The counter is returned raw as s32, hence the u32 signature.
 * - Making the fourth argument `u32 index`: 168 bytes (4 short). The
 *   `lsls r3,#24 / lsrs r3,#24` pair at entry disappears -- direct evidence for
 *   rule 15 that the argument really is u8.
 * - Taking the triple with `*(h->unk14->items + index)`: 6 differences. The
 *   counter-example to rule 28: in pointer form the `adds #116` offset is
 *   emitted BEFORE the multiplication, while the ROM has it AFTER. The
 *   array-index form is required.
 * - Reversing the order of the two assignments (payload first, then unk68):
 *   182 bytes, 166 differences. Rule 19 showed its sharpest effect in this
 *   function here: with the order changed, the r4 that ldmia clobbers is still
 *   live, the allocation shifts entirely and an extra stack slot opens.
 * - Removing the dead second argument from the signature (three parameters):
 *   5 differences. index falls to r2 instead of r3 and the narrowing order at
 *   entry breaks -- the dead argument really is in the signature.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_a7.c
 */

#include "gba_types.h"

#define ENTRY_COUNT   1     /* the number of slots in gRam020245B0 */
#define FAIL_INDEX    255   /* returned when there is no free slot */
#define ROOT_SLOT     64    /* the fixed index into gRom08BD3448.slots[] */
#define SUB_ARG       7     /* FUN_08014FFC's second argument */

/* So that the 12-byte triple at +0x4C is moved with a single ldmia/stmia pair
 * (rule 32); the same as the Triple in entries_a5.c / entries_b1.c. */
typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

/* The ROM root at 0x08BD3448 and the nodes chained from it; the same layout as
 * RomNode in src/world/entries_b1.c and src/world/entries_a6.c.
 * This translation unit reads only the +0x04 (sub-node table) and +0x14
 * fields. */
typedef struct RomNode {
    u32             pad00;
    struct RomNode **slots;     /* +0x04 */
    u8              pad08[12];
    u32             unk14;      /* +0x14 */
} RomNode;

extern RomNode gRom08BD3448;

/* The record at +0x18 of the object supplied by the caller; only +0x0C is
 * read, the rest is padding. */
typedef struct SourceState {
    u8  pad00[12];
    u32 unk0c;                  /* +0x0C -> the slot's +0x68 */
} SourceState;

/* The record held at +0x14 of the object supplied by the caller; from +0x74
 * onwards it holds an array of 12-byte elements (the stride was measured as 12
 * from the ROM's lsl/add/lsl). The array length is unknown, so it was left
 * flexible. */
typedef struct SourceTable {
    u8     pad00[116];
    Triple items[1];            /* +0x74 */
} SourceTable;

/* The object passed to the function. Only two pointer fields are read; these
 * offsets (0x14 / 0x18) overlap the sub-object (sub) region, but nothing more
 * could be proven in this translation unit, so no names were given. */
typedef struct Source {
    u8           pad00[20];
    SourceTable *unk14;         /* +0x14 */
    SourceState *unk18;         /* +0x18 */
} Source;

/* The single-entry table at 0x020245B0 (data/ram_map.csv: 148 bytes).
 * The layout is the same as Entry in the sibling files; only the fields
 * touched here were expanded. 148 = 0x94 in total. */
typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     pad01[3];
    u8     sub[38];             /* +0x04, passed to FUN_08014FFC/FUN_08015038 */
    u8     pad2a[34];
    Triple payload;             /* +0x4C */
    u8     pad58[16];
    u32    unk68;               /* +0x68 */
    u8     pad6c[40];           /* stride 148 */
} Entry;

extern Entry gRam020245B0[];

extern void FUN_08014ffc(u8 *dest, u32 count, Triple *src, u32 *value);
extern void FUN_08013cfc(u8 *dest, RomNode *desc, u32 arg);
extern void FUN_08014ee4(u8 *dest, u32 value);
extern void FUN_08015038(u8 *dest);

/* 0x08028B9C */
u32 SpawnEntryFromTable(Source *h, u32 unused1, u8 arg2, u8 index)
{
    Entry *e;
    RomNode *desc;
    s32 i;

    desc = gRom08BD3448.slots[ROOT_SLOT]->slots[0]->slots[0];

    for (i = 0; i < ENTRY_COUNT; i++) {
        if (gRam020245B0[i].active == 0)
            break;
    }

    if (i == ENTRY_COUNT)
        return FAIL_INDEX;

    e = &gRam020245B0[i];
    e->unk68 = h->unk18->unk0c;
    e->payload = h->unk14->items[index];

    FUN_08014ffc(e->sub, SUB_ARG, &e->payload, &e->unk68);
    FUN_08013cfc(e->sub, desc, arg2);
    FUN_08014ee4(e->sub, desc->unk14);
    FUN_08015038(e->sub);

    e->active = 1;
    return i;
}
