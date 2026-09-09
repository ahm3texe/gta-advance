/* Reset the area node and its history — 0x0805A298-0x0805A2F3
 *
 * The long form of src/script/cmd_area_reset.c: it clears the node with a 1
 * rather than a 0, raises the +0x17 byte, keeps only the low nibble of the
 * +0x0B flags, and then does three things with the +0x18 entry's kind byte --
 * hands it and its neighbour to the stub at 0x080673FC, zeroes the history
 * unless the kind is 6, and copies the save buffer's byte 8 to 9 for kinds 0
 * and 1 whose +0x16 handle resolves.
 *
 * Rule 33 for the nibble: the ROM materialises 15 first and ands the flags into
 * it (`movs r0,#15 / ldrb r1,[r4,#11] / ands r0,r1`).
 *
 * The +0x18 entry is re-read before each of the three uses; the ROM does not
 * keep it, and writing it into a local once produces one load instead of three.
 * The LAST read is the exception: the ROM keeps that one in r0 and passes it
 * straight to FUN_08031FF0, so the handle that call resolves belongs to the
 * ENTRY and not to the node. src/world/entry_index_or_none.c reads the same
 * +0x16 halfword from the other side.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_reset_area_full.c
 */

#include "gba_types.h"

#define FLAG_NIBBLE  15
#define KIND_NO_ZERO 6
#define KIND_MAX     1

typedef struct AreaEntry {
    u8 pad00[0x18];
    u8 kind;                    /* +0x18 */
    u8 next;                    /* +0x19 */
} AreaEntry;

typedef struct AreaNode {
    u8         pad00[11];
    u8         flags;           /* +0x0B */
    u8         pad0C[11];
    u8         ready;           /* +0x17 */
    AreaEntry *entry;           /* +0x18 */
} AreaNode;

extern AreaNode *FindOrInitAreaNode(u16 id);

extern void FUN_08032008(AreaNode *node, u32 value);
extern void SetSlot(u32 slot);
extern void FUN_080673fc(u32 kind, u32 next);
extern void ZeroHistory(void);
extern s32  FUN_08031ff0(AreaEntry *entry);
extern void SaveBufferCopy8To9(void);

/* 0x0805A298 */
u32 FUN_0805a298(u32 a, u16 id)
{
    AreaNode *node = FindOrInitAreaNode(id);
    AreaEntry *entry;
    u8 nibble;

    if (node == 0)
        return 1;
    FUN_08032008(node, 1);
    node->ready = 1;
    nibble = FLAG_NIBBLE;
    nibble &= node->flags;
    node->flags = nibble;
    SetSlot(0);
    FUN_080673fc(node->entry->kind, node->entry->next);
    if (node->entry->kind != KIND_NO_ZERO)
        ZeroHistory();
    entry = node->entry;
    if (entry->kind > KIND_MAX)
        return 1;
    if (FUN_08031ff0(entry) < 0)
        return 1;
    SaveBufferCopy8To9();
    return 1;
}
