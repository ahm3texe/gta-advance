/* Is one field below a percentage of another — 0x08059D10-0x08059D49
 *
 * Answers 1 when 100 times the +0x08 word is LESS than the second operand times
 * the +0x04 word, both taken from the record at +0x30 of the owner at +0x28.
 * That is the operand read as a percentage, with the multiplication kept in
 * integers rather than a division.
 *
 * The comparison is signed (`bge`), so both products are ints.
 *
 * Rule 53: `muls r1,r0` is the product written with the operands the other way
 * round in the source.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_below_percent.c
 */

#include "gba_types.h"

#define PERCENT  100

typedef struct Amounts {
    u8  pad00[4];
    s32 total;                  /* +0x04 */
    s32 current;                /* +0x08 */
} Amounts;

typedef struct AmountOwner {
    u8       pad00[0x30];
    Amounts *amounts;           /* +0x30 */
} AmountOwner;

typedef struct ChainNode {
    u8           pad00[0x28];
    AmountOwner *owner;         /* +0x28 */
} ChainNode;

extern ChainNode *FindRecordNodeEnd(u16 id);

/* 0x08059D10 */
u32 FUN_08059d10(u32 a, u16 id, u16 percent)
{
    ChainNode *node = FindRecordNodeEnd(id);
    AmountOwner *owner;
    Amounts *amounts;
    s32 have;
    s32 want;
    u32 result;

    if (node != 0) {
        owner = node->owner;
        if (owner != 0) goto found;
    }
    return 0;
found:
    result = 0;
    amounts = owner->amounts;
    have = PERCENT * amounts->current;
    want = percent * amounts->total;
    if (have < want)
        result = 1;
    return result;
}
