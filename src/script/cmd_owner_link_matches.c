/* Does one owner's link reach the other's owner — 0x08059D94-0x08059DEB
 *
 * Both operands are resolved to chain ends, and both must have an owner at
 * +0x28. The first owner's +0x14 record is then consulted: when its +0x114 byte
 * is above 3 the +0x100 word is taken, and otherwise nothing is, and the answer
 * is whether that equals the second owner.
 *
 * The two offsets are ONE variable in the ROM: 0x114 is built once
 * (`movs r3,#138 / lsls r3,#1`) and 0x100 is derived from it with `subs r3,#20`
 * (rule 65). Written as two separate constants the second is rebuilt.
 *
 * A byte of 3 or less answers with 0 compared against the owner, which can only
 * be false, so the low bytes are a "no link" case rather than a link to zero.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_owner_link_matches.c
 */

#include "gba_types.h"

#define KIND_OFFSET  (138 << 1) /* 0x114 */
#define LINK_OFFSET  (KIND_OFFSET - 20)
#define KIND_MIN     3

typedef struct LinkOwner {
    u8  pad00[0x14];
    u8 *record;                 /* +0x14 */
} LinkOwner;

typedef struct ChainNode {
    u8         pad00[0x28];
    LinkOwner *owner;           /* +0x28 */
} ChainNode;

extern ChainNode *FindRecordNodeEnd(u16 id);

/* 0x08059D94 */
u32 FUN_08059d94(u32 a, u16 first, u16 second)
{
    ChainNode *one = FindRecordNodeEnd(first);
    ChainNode *two;
    LinkOwner *owner;
    LinkOwner *other;
    u8 *record;
    u32 offset;
    u32 link;
    u32 result;

    if (one != 0) {
        two = FindRecordNodeEnd(second);
        if (two != 0) {
            owner = one->owner;
            if (owner != 0) {
                other = two->owner;
                if (other != 0) goto both;
            }
        }
    }
    return 0;
both:
    record = owner->record;
    offset = KIND_OFFSET;
    if (record[offset] > KIND_MIN) {
        offset -= 20;
        link = *(u32 *)(record + offset);
    } else {
        link = 0;
    }
    result = 0;
    if (link == (u32)other)
        result = 1;
    return result;
}
