/* Pass the area entry through the conditions and bind it to a node — 0x08052C68-0x08052CF7
 *
 * It takes the index-th entry from the array of 36-byte entries in the ROM
 * table (0x08D49C00 +0x24) and passes it through four gates:
 *   1. while gRam020004A0 is on, exit if bit 8 is set in the entry's +0x23 flag
 *      byte
 *   2. exit if the entry mask (+0x1C) is not -1 and does not intersect
 *      gRam02030C00
 *   3. exit if the +0x18 area flag is set (IsAreaFlagSet)
 *   4. exit if the +0x1A id does not pass IsProgressThresholdMet
 * If all are passed, the index is searched in the ordered node list
 * (gNodeListHead); if the upper half of the found node's +0x0B byte is 0x10,
 * FUN_08052988 is called.
 *
 * The pool layout (4 words, at 0x08052CE8):
 *     0x08D49C00  the ROM table base structure, +0x24 the entry array pointer
 *     0x020004A0  the flag gate's switch
 *     0x02030C00  the mask gate's switch
 *     0x02035A70  gNodeListHead
 *
 * The base is loaded PLAINLY in the ROM (`ldr r7,=0x08D49C00` +
 * `ldr r0,[r7,#36]`) and kept in r7 -> a STRUCT MEMBER access, not array
 * arithmetic. Measured: `extern Bank gAreaBank; gAreaBank.entries` produces
 * exactly that pair, while `((Bank*)0x08D49C00)->entries` folds the offset into
 * the pool constant.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 *
 * NEW SYMBOLS: 0x08D49C00 (gAreaBank), 0x020004A0 (gRam020004A0) and
 * 0x02030C00 (gRam02030C00) were NOT in data/ram_map.csv. They were not added
 * there because other agents were writing to the same file; the addresses are
 * file-scoped in this file.
 * The symbols are now recorded in data/ram_map.csv (added during the merge;
 * while the agent was running it was forbidden to write to the shared file and
 * it had temporarily used `asm(".equ ...")` -- that workaround was removed).
 * This preserves rule 1 (RAM/ROM addresses must be extern symbols): the
 * compiler cannot fold base+offset. Once they are in ram_map, these three lines
 * can be deleted.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_c3.c
 */

#include "gba_types.h"

/* See the top of the file: the three addresses we could not write to ram_map. */

/* A 36-byte area entry; only the fields that are used were named. */
typedef struct AreaEntry {
    u8  unk00[0x18];        /* 0x00 */
    u16 areaFlag;           /* 0x18 */
    u16 checkId;            /* 0x1A */
    s32 mask;               /* 0x1C */
    u8  unk20[3];           /* 0x20 */
    u8  flags;              /* 0x23 */
} AreaEntry;

typedef struct AreaBank {
    u8         unk00[0x24]; /* 0x00 */
    AreaEntry *entries;     /* 0x24 */
} AreaBank;

/* The same layout as the Node in src/world/node_search.c, with +0x0A/+0x0B added. */
typedef struct Node {
    struct Node *next;      /* 0x00 */
    u8           pad04[4];
    u16          id;        /* 0x08 */
    u8           pad0A;     /* 0x0A */
    u8           kind;      /* 0x0B */
} Node;

extern AreaBank gAreaBank;
extern u32      gRam020004A0;
extern u32      gRam02030C00;
extern Node    *gNodeListHead;

extern u32   IsAreaFlagSet(s32 index);
extern u32   IsProgressThresholdMet(s32 id);
extern Node *FindOrClaimNode(Node **head, s32 index);
extern void  FUN_08052988(Node *node, AreaEntry *entry);

/* 0x08052C68 */
void LinkAreaEntryIfEligible(s32 index)
{
    AreaEntry *entry;
    Node      *node;

    entry = &gAreaBank.entries[index];

    if (gRam020004A0 != 0) {
        if ((entry->flags & 8) != 0)
            return;
    }
    if (entry->mask != -1) {
        if ((entry->mask & gRam02030C00) == 0)
            return;
    }
    if (entry->areaFlag != 0) {
        if (IsAreaFlagSet(entry->areaFlag) != 0)
            return;
    }
    if (entry->checkId != 0) {
        if (IsProgressThresholdMet(entry->checkId) == 0)
            return;
    }

    node = FindOrClaimNode(&gNodeListHead, index);
    if (node == 0)
        return;
    if ((node->kind & 0xF0) != 0x10)
        return;

    FUN_08052988(node, &gAreaBank.entries[index]);
}
