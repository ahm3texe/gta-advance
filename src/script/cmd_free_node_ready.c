/* Is the free node ready — 0x08059AF8-0x08059B57
 *
 * Answers 1 when FUN_08056C80 says so for the owner at +0x28, and otherwise
 * falls back on a pair of tests that are the SAME in both arms: the owner must
 * have a slot, and gRam02035A9C must be at most 11. What differs between the
 * arms is only the guard ahead of them -- whether the +0x30 record is absent,
 * or present with a +0x18 kind of 4.
 *
 * The ROM duplicates those two tests rather than sharing them, which is what
 * writing them out twice gives; a shared tail would need a `goto` into it and
 * comes out shorter.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_free_node_ready.c
 */

#include "gba_types.h"

#define KIND_READY   4
#define COUNT_LIMIT  11

typedef struct ReadyRecord {
    u8 pad00[0x18];
    u8 kind;                    /* +0x18 */
} ReadyRecord;

typedef struct FreeNode {
    u8           pad00[0x28];
    u32          owner;         /* +0x28 */
    u8           pad2C[4];
    ReadyRecord *record;        /* +0x30 */
} FreeNode;

extern u8 gRam02035A9C;

extern FreeNode *FindFreeNode(u16 id);

extern u32 FUN_08056c80(u32 owner);
extern s32 GetOwnerSlot(u32 owner);

/* 0x08059AF8 */
u32 FUN_08059af8(u32 a, u16 id)
{
    FreeNode *node = FindFreeNode(id);
    ReadyRecord *record;

    if (node == 0) goto no;
    record = node->record;
    if (FUN_08056c80(node->owner) != 0) goto yes;
    if (record != 0) goto have;
    if (GetOwnerSlot(node->owner) == 0) goto no;
    if (gRam02035A9C <= COUNT_LIMIT) goto yes;
    goto no;
have:
    if (record->kind == KIND_READY) goto yes;
    if (GetOwnerSlot(node->owner) == 0) goto no;
    if (gRam02035A9C > COUNT_LIMIT) goto no;
yes:
    return 1;
no:
    return 0;
}
