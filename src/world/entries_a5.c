/* Finding a free slot in gEntriesA and setting up a new entry — 0x08028A88-0x08028B29
 *
 * Looks for the first free slot in the gEntriesA table (15 entries, 148-byte
 * stride) whose +0x00 field is zero.  If there is no free slot it returns
 * without doing anything.  When it finds one it fills the slot in: the active
 * flag 1, the kind (+0x64) 34, +0x90 = phase, +0x84 = owner, the 12-byte
 * triple supplied by the caller copied to +0x4c, +0x68 = arg1 << 16, +0x8c = 0.
 * Then FUN_08014ffc(&e->sub, 3, &e->payload, &e->unk68) is called; +0x2a gets
 * 1 if the phase is 51 and 64 otherwise; finally FUN_08015038(&e->sub) and
 * AdvanceEntryFrame(e) are called.
 *
 * Rule 35: `pop {r0}; bx r0` indicates a void return type.
 * Rule 32: `ldmia r0!,{r3,r6,r7}` / `stmia r1!,{r3,r6,r7}` requires writing
 *   the triple as a struct assignment (`Triple`), not as `*d++ = *s++`.
 * Rules 9/31: `cmp r4,#14` + a signed `bgt` branch -> the counter is `s32`.
 * Rule 42: the ROM walks a pointer in the loop, but an ascending indexed `for`
 *   was written in C; agbcc produces the pointer walker itself by strength
 *   reduction, while `&gEntriesA[i]` after the loop is rebuilt with
 *   `muls #148` -- exactly what the ROM does.
 *
 * The loop's exit branch goes straight to the epilogue (0x8028AA8 ->
 * 0x8028B20), because agbcc's jump threading skips the block that retests the
 * same condition (`i > 14`); writing a separate `break` + `if (i > 14) return;`
 * in the source produces that doubled `cmp`/`bgt` pattern.
 *
 * The base is loaded from the pool PLAIN (0x02023A00) and every field offset
 * is built separately with `adds` -> struct member access, not array
 * arithmetic; that is why the slot pointer is held as an `Entry *e`.
 *
 * MATCH: 162/162 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_a5.c
 */

#include "gba_types.h"

#define ENTRY_COUNT_MAX  14      /* i <= 14, i.e. 15 entries */
#define WANTED_KIND      34      /* +0x64 */
#define PHASE_SPECIAL    51      /* the separate phase == 51 branch */
#define MODE_SPECIAL     1       /* +0x2a */
#define MODE_DEFAULT     64      /* +0x2a */
#define SUB_ARG          3       /* FUN_08014ffc's second argument */

/* So the 12-byte triple at +0x4c is moved by a single `ldmia`/`stmia` pair
 * (rule 32). */
typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

/* The same layout as in the sibling files (entries_a1.c, entries_a3.c,
 * entries_a4.c, kind_scan.c); the fields this function touches were added.
 * The total size is 148 = 0x94. */
typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     pad01[3];
    u8     sub[38];             /* +0x04, passed to FUN_08014ffc/FUN_08015038 */
    u8     mode;                /* +0x2a */
    u8     pad2b[33];
    Triple payload;             /* +0x4c */
    u8     pad58[12];
    u8     kind;                /* +0x64 */
    u8     pad65[3];
    u32    unk68;               /* +0x68 */
    u8     pad6c[24];
    u32    unk84;               /* +0x84 */
    u8     pad88[4];
    u32    unk8c;               /* +0x8c */
    u32    unk90;               /* +0x90, stride 148 */
} Entry;

extern Entry gEntriesA[];

extern void FUN_08014ffc(u8 *dest, u32 count, Triple *src, u32 *value);
extern void FUN_08015038(u8 *sub);
extern void AdvanceEntryFrame(Entry *entry);

/* 0x08028A88 */
void CreateEntry(Triple *src, u32 arg1, u32 phase, u32 owner)
{
    Entry *e;
    s32 i;

    for (i = 0; i <= ENTRY_COUNT_MAX; i++) {
        if (gEntriesA[i].active == 0)
            break;
    }

    if (i > ENTRY_COUNT_MAX)
        return;

    e = &gEntriesA[i];
    e->active = 1;
    e->kind = WANTED_KIND;
    e->unk90 = phase;
    e->unk84 = owner;
    e->payload = *src;
    e->unk68 = arg1 << 16;
    e->unk8c = 0;

    FUN_08014ffc(e->sub, SUB_ARG, &e->payload, &e->unk68);

    if (phase == PHASE_SPECIAL)
        e->mode = MODE_SPECIAL;
    else
        e->mode = MODE_DEFAULT;

    FUN_08015038(e->sub);
    AdvanceEntryFrame(e);
}
