/* Clear the area and rebuild the id array — 0x08052E78-0x0805302F
 * (440 bytes; the last 22 are a literal pool)
 *
 * The descriptor block at the area's +0x28 carries two things: an entry count
 * at +0x05 and a u16 id array (the template) at +0x0C. The function first
 * clears the area's level nibble, then, for every id in the template:
 *
 *   1) It takes the 36-byte area entry from the ROM bank (gAreaBank +0x24) and
 *      walks the +0x10 slot array for as many entries as the +0x06 slot count.
 *      For each slot id:
 *        - the marker object is looked up with FindFreeNode and, if found, its
 *          +0x18 flag word is masked with 0xE3FF0000,
 *        - ReleaseAreaNode is called,
 *        - the 64-byte sub-record in the bank's +0x1C field is taken under the
 *          same id; if its +0x0D slot count is nonzero, its +0x10 array is
 *          walked with the same two steps.
 *   2) The id is searched in the ordered node list (FindNode). If the node
 *      exists:
 *        - if it is dirty (bit 0), the node's own slot array is walked with
 *          ReleaseAreaNode, pulled to the empty id with FillSlotsWithNone and
 *          the dirty bit cleared (the same body as nodelist_c2.c / d1.c),
 *        - if its level is GREATER than 1, it is pulled down to 1,
 *        - gNodeListHead is refreshed with that id.
 *
 * Finally, if the area has a +0x30 output array, the template is walked again
 * and every id written there; ids greater than 0x7FFF are passed through the
 * remapping table at the area's +0x18 (indexed by id - 0x8000). The written id
 * is passed to FUN_08052C68.
 *
 * STRUCTURE HINTS:
 *   - gAreaBank (0x08D49C00) +0x24 = the array of 36-byte entries
 *     (the same as in src/core/nodelist_c3.c and src/core/nodelist_d5.c),
 *     +0x1C = the array of 64-byte sub-records (SEEN FOR THE FIRST TIME IN
 *     THIS FILE).
 *     In the ROM the base is loaded PLAINLY and the offset left in the load
 *     instruction (`ldr r2,=0x08D49C00` + `ldr r1,[r2,#36]`), i.e. a STRUCT
 *     MEMBER access; writing a constant cast folds the offset into the pool
 *     constant (rule 1).
 *   - The area's descriptor block is the same as the SlotDesc in
 *     src/core/nodelist_d2.c: +0x05 countA (the area's id count), +0x06 countB
 *     (the node's slot count). The +0x0C template array was added in this
 *     file.
 *
 * RULES APPLIED:
 *   - Rule 1: gAreaBank / gNodeListHead are extern symbols.
 *   - Rule 2: an INTERMEDIATE POINTER in the form
 *     `entry = &gAreaBank.entries[id]`; the ROM keeps the base in r7 and
 *     reaches the members with `ldr r0,[r7,#16]` / `ldrb r1,[r7,#6]`.
 *   - Rule 9/31: the counters are `int`; the ROM emits `blt`/`bge` (signed).
 *     The 0x7FFF comparison, by contrast, is `bls` (UNSIGNED) in the ROM, so
 *     the id local there is `u32`.
 *   - Rule 24/26: the +0x0B bytes of the area and the node are bitfields.
 *     `area->level = 0` -> `movs #15 / ands / strb`;
 *     `node->dirty = 0` -> `movs #2 / negs`;
 *     `node->level > 1` -> a SIGNED 4-bit field, `lsls #24 / asrs #28`.
 *   - Rule 11: the id crosses the FindNode call -> a separate `u32` local
 *     (the ROM keeps it in r7; the `ldrh r7,[r2]` + `adds r0,r7,#0` order is
 *     the `u32` direction of the measurement in nodelist_d3.c).
 *   - Rule 35: `pop {r0}; bx r0` -> a void return type.
 *   - The inner slot arrays are walked BY INDEX, not by ADVANCING A POINTER
 *     (the ROM builds j*2 once with `lsls r4,r6,#1` and shares it across three
 *     accesses); the node's own array, on the other hand, is advanced
 *     (`adds r5,#2`).
 *
 * THE ONE MEASURED DETAIL — the INVERSE DIRECTION of rule 43 (16 differences
 * -> 0):
 *   Written as `i++, ids++`, the first outer loop's increments left a 16-byte
 *   difference; the instructions were right, only their ORDER was reversed. In
 *   this loop the increments are hoisted to the TOP of the body and spilled to
 *   the stack (sp+4 = i+1, sp+8 = ids+2), then read back at the bottom. agbcc
 *   emits those two carriers IN SOURCE ORDER, so the ROM's
 *       mov r0,r8 / adds r0,#2 / str r0,[sp,#8] / adds r5,#1 / str r5,[sp,#4]
 *   order only comes out with `ids++, i++`. In the second outer loop, by
 *   contrast, the ROM increments the counter first, and there `i++, out++,
 *   ids++` is the right order.
 *   So rule 43 cannot be memorised as "counter first": in every loop the ROM's
 *   own increment order must be read and written into the source in that
 *   order.
 */

#include "gba_types.h"

#define LEVEL_BASE   1
#define MARKER_KEEP  0xE3FF0000
#define REMAP_BASE   0x8000

/* The same as the SlotDesc in src/core/nodelist_d2.c, with +0x0C added. */
typedef struct SlotDesc {
    u8   pad00[5];
    u8   countA;                /* +0x05 the area's id count */
    u8   countB;                /* +0x06 the node's slot count */
    u8   pad07[5];
    u16 *ids;                   /* +0x0C the template id array */
} SlotDesc;

/* The same layout as src/core/nodelist_d1.c / d5.c. */
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

/* An object on the 0x02035A80 list; only its +0x18 flag word is used here. */
typedef struct Marker {
    u8  pad00[0x18];
    u32 flags;                  /* +0x18 */
} Marker;

/* A 36-byte area entry; the same as in src/core/nodelist_c3.c, with +0x06 and
 * +0x10 named in this file. */
typedef struct AreaEntry {
    u8   pad00[6];
    u8   count;                 /* +0x06 */
    u8   pad07[9];
    u16 *slots;                 /* +0x10 */
    u8   pad14[4];
    u16  areaFlag;              /* +0x18 */
    u16  checkId;               /* +0x1A */
    s32  mask;                  /* +0x1C */
    u8   unk20[3];
    u8   flags;                 /* +0x23 */
} AreaEntry;

/* 64-byte subrecord. */
typedef struct AreaSub {
    u8   pad00[13];
    u8   count;                 /* +0x0D */
    u8   pad0E[2];
    u16 *slots;                 /* +0x10 */
    u8   pad14[0x2C];
} AreaSub;

typedef struct AreaBank {
    u8         unk00[0x1C];
    AreaSub   *subs;            /* +0x1C */
    u8         pad20[4];
    AreaEntry *entries;         /* +0x24 */
} AreaBank;

typedef struct Area {
    u8        pad00[11];
    u8        pad0B : 4;        /* +0x0B bit 0..3 */
    s8        level : 4;        /* +0x0B bit 4..7 */
    u8        pad0C[12];
    u16       remap[8];         /* +0x18 */
    SlotDesc *desc;             /* +0x28 */
    u8        pad2C[4];
    u16      *out;              /* +0x30 */
} Area;

extern AreaBank gAreaBank;
extern Node    *gNodeListHead;

extern Marker *FindFreeNode(u32 id);
extern Node   *FindNode(u32 id);
extern void    ReleaseAreaNode(u32 a, u32 b);
extern void    FillSlotsWithNone(void *dest, u32 count);
extern void    FUN_08055d90(u32 *head, u32 id);
extern void    LinkAreaEntryIfEligible(s32 index);

/* 0x08052E78 */
void ResetAreaIds(Area *area)
{
    SlotDesc  *desc;
    int        i;
    int        j;
    int        k;
    int        m;
    u16       *ids;
    u16       *out;
    u16       *slot;
    AreaEntry *entry;
    AreaSub   *sub;
    Marker    *marker;
    Node      *node;
    u32        id;
    u32        value;

    desc = area->desc;
    area->level = 0;
    if (desc->countA == 0)
        return;

    ids = desc->ids;
    for (i = 0; i < desc->countA; ids++, i++) {
        entry = &gAreaBank.entries[*ids];

        for (j = 0; j < entry->count; j++) {
            marker = FindFreeNode(entry->slots[j]);
            if (marker != 0)
                marker->flags &= MARKER_KEEP;
            ReleaseAreaNode(entry->slots[j], 0);

            sub = &gAreaBank.subs[entry->slots[j]];
            if (sub->count == 0)
                continue;

            for (k = 0; k < sub->count; k++) {
                marker = FindFreeNode(sub->slots[k]);
                if (marker != 0)
                    marker->flags &= MARKER_KEEP;
                ReleaseAreaNode(sub->slots[k], 0);
            }
        }

        id = *ids;
        node = FindNode(id);
        if (node == 0)
            continue;

        if (node->dirty) {
            slot = node->slots;
            for (m = 0; m < node->desc->countB; m++, slot++)
                ReleaseAreaNode(*slot, 0);
            FillSlotsWithNone(node->slots, node->desc->countB);
            node->dirty = 0;
        }

        if (node->level > LEVEL_BASE)
            node->level = LEVEL_BASE;

        FUN_08055d90((u32 *)&gNodeListHead, id);
    }

    out = area->out;
    if (out == 0)
        return;

    ids = desc->ids;
    for (i = 0; i < desc->countA; i++, out++, ids++) {
        value = *ids;
        if (value > 0x7FFF)
            value = area->remap[value - REMAP_BASE];
        *out = value;
        LinkAreaEntryIfEligible(*out);
    }
}
