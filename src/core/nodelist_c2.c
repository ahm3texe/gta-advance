/* Empty the node's slots and refresh the list — 0x08055C04-0x08055C8B
 *
 * It finds the node from the id. If the node is "dirty" (bit 0) and either its
 * level is 1 or the force flag was given: ReleaseAreaNode is called for every
 * id in the node's slot array, after which the slots are filled with the empty
 * id via DMA and the dirty bit is cleared. With force, a level above 1 is
 * pulled down to 1.
 * At the end FUN_08055D90 is called with the list head + id.
 *
 * BITFIELD AT +0x0B: the `ldrb` + `lsls #24` / `asrs #28` pair for bits 4..7
 * means a SIGNED 4-bit field. The `== 1` comparison on the same field, by
 * contrast, produces `movs #240 / ands / cmp #16` with no shift — agbcc
 * reducing a bitfield equality comparison to a mask. The `level = 1` assignment
 * gives `movs #15 / ands / movs #16 / orrs`; all three match the ROM
 * exactly.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 * Rule 43: the counter and the pointer advance together -> both in the `for`
 * increment, in the ROM's order (`adds r5,#1` then `adds r4,#2`).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_c2.c
 */

#include "gba_types.h"

#define LEVEL_BASE  1

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

extern Node *gNodeListHead;         /* 0x02035A70 */

extern Node *FindNode(u32 id);
extern void  ReleaseAreaNode(u32 a, u32 b);
extern void  FillSlotsWithNone(void *dest, u32 count);
extern void  FUN_08055d90(u32 *head, u32 id);

/* 0x08055C04 */
void ReleaseNodeSlots(u32 id, u32 force)
{
    Node *node;
    u16  *slot;
    int   i;

    node = FindNode(id);
    if (node == 0)
        return;

    if (node->dirty) {
        if (node->level == LEVEL_BASE || force != 0) {
            slot = node->slots;
            for (i = 0; i < node->desc->count; i++, slot++)
                ReleaseAreaNode(*slot, 0);
            FillSlotsWithNone(node->slots, node->desc->count);
            node->dirty = 0;
        }
    }

    if (force != 0) {
        if (node->level > LEVEL_BASE)
            node->level = LEVEL_BASE;
    }

    FUN_08055d90((u32 *)&gNodeListHead, id);
}
