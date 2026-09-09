/* Load/unload areas when the active region mask is refreshed
 * 0x0805518C-0x080552D3  (328 bytes; the last 4 are a literal pool)
 *
 * A region index is computed from the player's (or players') world coordinate
 * and a mask is built with `1 << index`. With two players (gGameState[12]) the
 * second player's bit is added as well. If there is no player object the mask
 * is -1.
 *
 * The new mask is written to gRam02030C00. If the mask is -1, or if it equals
 * the PREVIOUS mask, the work is done. If it changed, the id array of the
 * record group given by GetRecordIndex() is scanned; for each id, the +0x1C
 * mask of the 36-byte area entry in the ROM bank (0x08D49C00 +0x24) is
 * examined:
 *
 *   - entry mask -1                     -> skip
 *   - intersects the OLD mask           -> the area is no longer visible:
 *       if it also intersects the new mask, skip; otherwise UNLOAD
 *       (FindNode + clear the slots if dirty/level-1 + FUN_08055D90)
 *   - does not intersect the old mask   -> the area has just appeared:
 *       if it intersects the new mask, LOAD (FUN_08052C68)
 *
 * The unload body is byte-for-byte the same as FUN_08055C04 in
 * src/core/nodelist_c2.c: two SEPARATE tests through a single `ldrb`
 * (`movs #1 / ands` and `movs #240 / ands / cmp #16`), the inner slot loop,
 * FillSlotsWithNone, and clearing the dirty bit with `movs #2 / negs`.
 *
 * STRUCTURE HINTS:
 *   - The +0x2C field of the 0x08D49C00 bank is an array of 16-byte "record
 *     groups": +0x04 u8 id count, +0x08 u16 id array. In the ROM the base is
 *     loaded PLAINLY and the offset left in the load instruction
 *     (`ldr r1,=0x08D49C00` + `ldr r1,[r1,#44]`), i.e. a STRUCT MEMBER access
 *     -> an extern object.
 *     src/core/nodelist_c3.c uses the +0x24 field of the same bank.
 *   - The player slots (gRam02000F10 / gRam02001140) carry a pointer at
 *     +0x00; that object's +0x18 is a {s32 x, s32 y} coordinate.
 *
 * FOUR MEASURED DETAILS (each was tried on its own; all four are decisive):
 *
 * 1) A SEPARATE LOCAL FOR THE MASK ADDRESS, DECLARED AFTER THE READ.
 *    The ROM loads the address from the pool once (r1), reads the value, then
 *    COPIES the address with `adds r6,r1,#0`, keeps it in r6 across the calls
 *    and does the final store from there. Writing `gRam02030C00 = ...` plainly
 *    puts a second pool load in front of the store; the code shrinks by 2
 *    bytes, the pool alignment padding adds 2 bytes, for a net 332/218
 *    differences. Writing the `cur = &gRam02030C00;` line AFTER THE READ (rule
 *    19: source order is preserved) makes CSE turn that assignment into a copy
 *    of the first load -- which is exactly the ROM's `adds r6,r1,#0`.
 *    Writing the line BEFORE the read produces a single pseudo
 *    (`ldr r6,pool`) and no copy comes out.
 *
 * 2) TWO SEPARATE LOCALS FOR THE RESULT: `bits` and `active`. The ROM's -1
 *    branch builds r1 and the computed branch r4, merging them with
 *    `adds r1,r4,#0` at the end of the else branch; the -1 branch jumps OVER
 *    that copy (to 0x080551E2). Written with a single variable, agbcc gave
 *    both branches r4 and never produced the copy (127/328 differences).
 *    Computing `bits` in the else branch and writing `active = bits;` at its
 *    end produced the copy and brought the difference down to 8.
 *
 * 3) THE ID LOCAL MUST BE `u32`, NOT `u16`. With a `u16 id`, instead of the
 *    ROM's single `ldrh r7,[r0]` the pair `ldrh r2,[r0]` + `adds r7,r2,#0`
 *    comes out: an HImode local widens into a separate SImode pseudo for both
 *    the index computation and the call argument. This is the INVERSE
 *    direction of the rule measured in src/core/nodelist_d3.c -- there the ROM
 *    used two registers, so `u16` was required. So the width of an id local is
 *    not memorized; it follows from how many registers the ROM uses.
 *
 * 4) A SEPARATE POINTER LOCAL FOR THE AREA ENTRY. Written as
 *    `mask = gAreaBank.entries[id].mask;`, agbcc loads the `entries` member
 *    first, then scales id*36 and leaves the sum in the INDEX register. The
 *    ROM does the opposite: id*36 first, then `ldr r0,[r0,#36]`, with the sum
 *    in the BASE register. The gap is 4 instructions / 8 bytes.
 *    `entry = &gAreaBank.entries[id];` produces a separate address computation
 *    (an ADDR_EXPR), so the order and register allocation fall into the ROM's.
 *    A verified example of the same pattern is FUN_08052C68 in
 *    src/core/nodelist_c3.c: there too the form is
 *    `ldr r7,pool / lsls / adds / lsls / ldr r0,[r7,#36] / adds r4,r0,r6`.
 *    Writing `(gAreaBank.entries + id)->mask` changed nothing (the same tree,
 *    the same RTL).
 *
 * OTHER RULES APPLIED:
 *   - Rule 1: gRam02030C00 / gNodeListHead / gAreaBank are extern symbols.
 *     Writing `((AreaBank *)0x08D49C00)->groups` folds the offset into the
 *     pool constant (measured in nodelist_c3.c).
 *   - Rule 9/31: the counters are `int`; the ROM emits `bge` / `blt` (signed).
 *   - Rule 24/26: the node's +0x0B byte is a bitfield; the `dirty = 0`
 *     assignment gives the `movs #2 / negs` pair.
 *   - Rule 35: `pop {r0}; bx r0` -> a void return type.
 *   - Both `1 << ...` derive from the same constant; the ROM shares the
 *     constant in r5 and emits `adds r4,r5,#0` -- there is no need to write
 *     separate `movs #1`s, writing `1 <<` twice in the source is enough.
 *   - The inner slot loop and the clearing of the dirty bit come from the same
 *     source as src/core/nodelist_c2.c.
 *
 * MATCH: 328/328 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_d5.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define LEVEL_BASE  1

/* The world coordinate at the player object's +0x18. */
typedef struct Coord {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
} Coord;

typedef struct Racer {
    u8     pad00[0x18];
    Coord *coord;               /* +0x18 */
} Racer;

/* The same layout as src/core/nodelist_d1.c / nodelist_c2.c. */
typedef struct SlotDesc {
    u8 pad00[6];
    u8 count;                   /* +0x06 */
} SlotDesc;

typedef struct Node {
    struct Node *next;          /* +0x00 */
    u8           pad04[4];
    u16          id;            /* +0x08 */
    u8           pad0A;
    u8           dirty : 1;     /* +0x0B bit 0    */
    u8           pad0B : 3;     /* +0x0B bit 1..3 */
    s8           level : 4;     /* +0x0B bit 4..7 */
    u8           pad0C[8];
    SlotDesc    *desc;          /* +0x14 */
    u16         *slots;         /* +0x18 */
} Node;

/* A 36-byte area entry; the same as in src/core/nodelist_c3.c. */
typedef struct AreaEntry {
    u8  unk00[0x18];            /* 0x00 */
    u16 areaFlag;               /* 0x18 */
    u16 checkId;                /* 0x1A */
    s32 mask;                   /* 0x1C */
    u8  unk20[3];               /* 0x20 */
    u8  flags;                  /* 0x23 */
} AreaEntry;

/* A 16-byte record group. */
typedef struct AreaGroup {
    u8   pad00[4];              /* 0x00 */
    u8   count;                 /* 0x04 */
    u8   pad05[3];              /* 0x05 */
    u16 *ids;                   /* 0x08 */
    u8   pad0C[4];              /* 0x0C */
} AreaGroup;

typedef struct AreaBank {
    u8         unk00[0x24];     /* 0x00 */
    AreaEntry *entries;         /* 0x24 */
    u8         pad28[4];        /* 0x28 */
    AreaGroup *groups;          /* 0x2C */
} AreaBank;

extern AreaBank gAreaBank;
extern u32      gRam02030C00;
extern Node    *gNodeListHead;
extern u8       gGameState[16];

extern u32   FUN_080313f0(s32 x, s32 y);
extern u32   GetRecordIndex(void);
extern Node *FindNode(u32 id);
extern void  ReleaseAreaNode(u32 a, u32 b);
extern void  FillSlotsWithNone(void *dest, u32 count);
extern void  FUN_08055d90(u32 *head, u32 id);
extern void  LinkAreaEntryIfEligible(s32 index);

/* 0x0805518C */
void RefreshActiveAreas(void)
{
    AreaGroup *group;
    AreaEntry *entry;
    Node      *node;
    Racer     *racer;
    u16       *slot;
    u32       *cur;
    int        i;
    int        j;
    u32        id;
    s32        mask;
    s32        bits;
    s32        active;
    u32        old;

    old = gRam02030C00;
    racer = *(Racer **)gRam02000F10;
    cur = &gRam02030C00;        /* AFTER THE READ: CSE turns this into a
                                   copy of the first pool load; see point 1
                                   in the header comment */
    if (racer == 0) {
        active = -1;
    } else {
        bits = 1 << FUN_080313f0(racer->coord->x, racer->coord->y);
        if (gGameState[12] != 0) {
            racer = *(Racer **)gRam02001140;
            bits |= 1 << FUN_080313f0(racer->coord->x, racer->coord->y);
        }
        active = bits;          /* a separate local so the two branches merge
                                   at the SHARED store; header point 2 */
    }
    *cur = active;

    if (active == -1)
        return;
    if (old == active)
        return;

    group = &gAreaBank.groups[GetRecordIndex()];

    for (i = 0; i < group->count; i++) {
        id = group->ids[i];
        /* A separate pointer local: only this way do the order and the
           register allocation match the ROM's (header point 4). */
        entry = &gAreaBank.entries[id];
        mask = entry->mask;
        if (mask == -1)
            continue;

        if ((old & mask) != 0) {
            if ((gRam02030C00 & mask) != 0)
                continue;

            node = FindNode(id);
            if (node == 0)
                continue;

            if (node->dirty) {
                if (node->level == LEVEL_BASE) {
                    slot = node->slots;
                    for (j = 0; j < node->desc->count; j++, slot++)
                        ReleaseAreaNode(*slot, 0);
                    FillSlotsWithNone(node->slots, node->desc->count);
                    node->dirty = 0;
                }
            }

            FUN_08055d90((u32 *)&gNodeListHead, id);
        } else {
            if ((gRam02030C00 & mask) != 0)
                LinkAreaEntryIfEligible(id);
        }
    }
}
