/* Opening a free slot in gEntriesA and building a new entry from a template
 * — 0x08025424-0x08025515
 *
 * The same family as its sibling src/world/entries_a5.c (0x08028A88): both
 * search the gEntriesA table (15 entries, 148-byte stride) for the first free
 * slot whose +0x00 field is zero.  This version differs in that it
 *   - fills the slot by copying FROM AN EXISTING ENTRY (src),
 *   - performs a three-level resolution through the ROM table (gRom08BD3448)
 *     and attaches the descriptor it obtains to the sub-object,
 *   - returns 0 if it finds no free slot and 1 if it builds one.
 *
 * The flow:
 *   desc = gRom08BD3448.slots[kind]->slots[phase]->slots[0]
 *   return 0 if there is no free slot
 *   e->kind/phase/+0x66 are set; the +0x4C triple, +0x68 and +0x84 are copied
 *     from src
 *   FUN_08014FFC / FUN_08013CFC / FUN_08014EE4 / FUN_08015038 set up the
 *     sub-object
 *   e->active=1, +0x8C=0, timer=125, owner=arg5
 *   mode 64 if the phase is 47, otherwise 16
 *
 * THE SIGNATURE was confirmed from the call sites (0x08025662 and 0x0802568C):
 * six arguments are passed (r0, 1, 0, 34, [sp,#0], [sp,#4]).  The second and
 * third arguments are NEVER READ by the callee; on entry the ROM moves r0 into
 * r2 with `adds r2,r0,#0` and clobbers r1/r2.  So they stay in the signature
 * but are unused.
 *
 * THE PARAMETER TYPES were read from the instruction sequence (rule 15): the
 * fourth and sixth arguments are narrowed with a `lsls #24 / lsrs #24` pair ->
 * UNSIGNED 8-bit; the fifth is used as a raw word -> u32.
 *
 * The reverse of rule 35: the epilogue `pop {r1}; bx r1` — the return address
 * is taken into r1, so r0 is live and the function returns a VALUE (u32).
 *
 * Rule 32: the 12 bytes at +0x4C are moved by an
 * `ldmia r0!,{r3,r4,r7}` / `stmia r1!,{...}` pair; that only comes out of a
 * struct assignment (Triple), while `*d++ = *s++` triples give three separate
 * ldr/str.
 *
 * Rules 9/31: the in-loop branch is `cmp #14` + `bgt`, i.e. SIGNED -> the
 * counter is `s32`.  The branch after the loop, however, is `cmp #15` + `bne`;
 * that corresponds to `i == 15`, not `i > 14` (written as `i > 14`, agbcc
 * again produces `cmp #14`/`bgt` and the block order shifts).
 *
 * Rule 19: while filling the slot, the source order decides the ROM's
 * `adds`/`subs` chain: +0x64 (adds #100), +0x90 (adds #44), +0x66 (subs #42).
 * Change the order and the offset chain changes too.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_b1.c
 */

#include "gba_types.h"

#define ENTRY_COUNT     15      /* the number of slots in gEntriesA */
#define SUB_ARG         131     /* FUN_08014FFC's second argument (0x83) */
#define TIMER_START     125     /* the +0x02 initial value */
#define PHASE_SPECIAL   47      /* the separate +0x90 == 47 branch */
#define MODE_SPECIAL    64      /* +0x2A */
#define MODE_DEFAULT    16      /* +0x2A */

/* So the 12-byte triple at +0x4C is moved by a single ldmia/stmia pair
 * (rule 32); the same as the Triple in entries_a5.c. */
typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

/* The ROM root at 0x08BD3448 and the nodes chained from it.  The root and the
 * intermediate nodes have the SAME shape: a pointer to a pointer array at
 * +0x04.  So a single recursive type is enough.  The same symbol is declared
 * with a SHALLOWER view (GlyphRoot/GlyphEntry) in
 * src/text/glyph_table_access.c; the +0x14 field there is read here too, so it
 * is the same physical node.  Because this translation unit needs a different
 * depth, the view is kept separate. */
typedef struct RomNode {
    u32             pad00;
    struct RomNode **slots;     /* +0x04 */
    u8              pad08[12];
    u32             unk14;      /* +0x14 */
} RomNode;

extern RomNode gRom08BD3448;

/* The same layout as in the sibling files (entries_a1.c ... entries_a5.c,
 * kind_scan.c); the fields this function touches have been named.  The total
 * is 148 = 0x94. */
typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     owner;               /* +0x01 */
    u16    timer;               /* +0x02 */
    u8     sub[38];             /* +0x04, passed to FUN_08014FFC/FUN_08015038 */
    u8     mode;                /* +0x2A */
    u8     pad2b[33];
    Triple payload;             /* +0x4C */
    u8     pad58[12];
    u8     kind;                /* +0x64 */
    u8     pad65;
    u16    unk66;               /* +0x66 */
    u32    unk68;               /* +0x68 */
    u8     pad6c[24];
    u32    unk84;               /* +0x84 */
    u8     pad88[4];
    u32    unk8c;               /* +0x8C */
    u32    phase;               /* +0x90, stride 148 */
} Entry;

extern Entry gEntriesA[];

extern void FUN_08014ffc(u8 *dest, u32 count, Triple *src, u32 *value);
extern void FUN_08013cfc(u8 *dest, RomNode *desc, u32 arg);
extern void FUN_08014ee4(u8 *dest, u32 value);
extern void FUN_08015038(u8 *dest);

/* 0x08025424 */
u32 CreateEntryFromTemplate(Entry *src, u32 unused1, u32 unused2, u8 kind, u32 phase,
                 u8 owner)
{
    Entry *e;
    RomNode *desc;
    s32 i;

    desc = gRom08BD3448.slots[kind]->slots[phase]->slots[0];

    for (i = 0; i < ENTRY_COUNT; i++) {
        if (gEntriesA[i].active == 0)
            break;
    }

    if (i == ENTRY_COUNT)
        return 0;

    e = &gEntriesA[i];
    e->kind = kind;
    e->phase = phase;
    e->unk66 = 0;
    e->payload = src->payload;
    e->unk68 = src->unk68;
    e->unk84 = src->unk84;

    FUN_08014ffc(e->sub, SUB_ARG, &e->payload, &e->unk68);
    FUN_08013cfc(e->sub, desc, 0);
    FUN_08014ee4(e->sub, desc->unk14);
    FUN_08015038(e->sub);

    e->active = 1;
    e->unk8c = 0;
    e->timer = TIMER_START;
    e->owner = owner;

    if (phase == PHASE_SPECIAL)
        e->mode = MODE_SPECIAL;
    else
        e->mode = MODE_DEFAULT;

    return 1;
}
