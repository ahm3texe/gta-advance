/* Rebuild the area entry and return the node. 0x080526B8, 152 bytes.
 *
 * A sibling of FUN_08052C68 (src/core/nodelist_c3.c): it uses the same
 * "flag bit 8 gates while gRam020004A0 is on" guard and the same
 * `(kind & 0xF0) == 0x10` test. The difference: when the gate is not hit, it
 * clears two bits from the node's +0x0B flags, rebuilds the counter and RETURNS
 * the node.
 *
 * CAREFUL -- there are TWO SEPARATE lists. The head pointer passed here is
 * gList02035A80 (0x02035A80), NOT the gNodeListHead (0x02035A70) that
 * nodelist_c3.c uses. The ram_map note records that the two are separate
 * objects; because their addresses are neighbours they are easily confused.
 *
 * gAreaBank is seen here as +0x04 (a counter) and +0x1C (an array of 64-byte
 * records); nodelist_c3.c sees the same symbol as the array of 36-byte entries
 * at +0x24. Both are correct: each translation unit casts to its own local view
 * (see the header of include/ram_symbols.h).
 *
 * The mask: the ROM builds -13 with `movs r0,#13 / negs r0,r0`. The field's
 * signedness MAKES NO DIFFERENCE here (it matches with u8 too), because in the
 * compound assignment `&= ~12` the operation happens at int width and `~12` is
 * already -13.
 * The case where rule 47 does apply is different: when the result is used
 * NARROWED to a small type (as in ResetActor), it folds to the constant 0xF0 on
 * a u8 field.
 *
 * AN ELIMINATED FORM: taking the result into an `s32` local first and writing
 * it back to the field. agbcc then inserts an `lsls #24 / asrs #24`
 * normalization before the store -- two extra instructions the ROM does not
 * have. The direct compound assignment is correct.
 *
 * The return type: on exit the ROM does `adds r0,r4,#0` and returns with
 * `pop {r1}; bx r1`, so r0 is LIVE -> a value-returning function.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_b6.c
 */

#include "gba_types.h"

#define KIND_CLEAR  12          /* two bits cleared at +0x0B */
#define KIND_MASK   0xF0
#define KIND_READY  0x10
#define SPAN_NUM    15
#define SPAN_SHIFT  2
#define SPAN_BIAS   9
#define BITS_SKIP   0x80
#define RECORD_SIZE 0x40

typedef struct Shape {
    u8  pad00[10];
    u16 span;                   /* 0x0A */
} Shape;

typedef struct Node {
    struct Node *next;          /* 0x00 */
    u8    pad04[4];
    u16   id;                   /* 0x08 */
    u8    pad0A;
    s8    kind;                 /* 0x0B */
    u8    pad0C[8];
    s16   timer;                /* 0x14 */
    u8    pad16[2];
    u32   bits;                 /* 0x18 */
    u8    pad1C[8];
    u32   value;                /* 0x24 */
    u8    pad28[4];
    Shape *shape;               /* 0x2C */
} Node;

/* A 64-byte area record; only the field that is used was named. */
typedef struct Record {
    u8 pad00[0x3d];
    u8 flags;                   /* 0x3D */
    u8 pad3e[2];
} Record;

typedef struct AreaBank {
    u8      pad00[4];
    s32     count;              /* 0x04 */
    u8      pad08[0x14];
    Record *records;            /* 0x1C */
} AreaBank;

extern AreaBank gAreaBank;
extern u32      gRam020004A0;
extern Node    *gList02035A80;

extern Node *FindOrClaimNode(Node **head, s32 index);
extern void  FUN_080521c4(Node *node, Record *record);

/* 0x080526B8 */
Node *RebuildAreaEntry(s32 index, u32 value)
{
    Node *node;

    if (index >= gAreaBank.count) return 0;
    if (gRam020004A0 != 0) {
        if ((gAreaBank.records[index].flags & 8) != 0) return 0;
    }

    node = FindOrClaimNode(&gList02035A80, index);

    node->kind &= ~KIND_CLEAR;
    if ((node->kind & 2) != 0) {
        FUN_080521c4(node, &gAreaBank.records[index]);
    }

    if ((node->kind & KIND_MASK) == KIND_READY) {
        node->value = value;
        node->timer = ((node->shape->span * SPAN_NUM) >> SPAN_SHIFT) + SPAN_BIAS;
        if ((node->bits & BITS_SKIP) != 0) node->timer = 0;
    }
    return node;
}
