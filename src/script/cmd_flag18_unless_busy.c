/* Script command: bit 18 of the node flags unless the owner is busy
 * 0x08059D4C-0x08059D7B
 *
 * Answers 0 when the operand resolves to nothing, and also when the node HAS an
 * owner at +0x28 and FUN_0803CA58 says something about it. In every other case,
 * including the one where there is no owner at all, the answer is bit 18 of the
 * node's own +0x18 flags.
 *
 * The zero answer sits BETWEEN the tests and the flag read in the ROM, with the
 * tests falling through into it, so the flag read is the body reached by the two
 * forward branches and the source is written the same way round.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_flag18_unless_busy.c
 */

#include "gba_types.h"

typedef struct BusyOwner BusyOwner;

typedef struct FlagNode {
    u8         pad00[0x18];
    u32        flags;           /* +0x18 */
    u8         pad1c[0x0C];
    BusyOwner *owner;           /* +0x28 */
} FlagNode;

extern FlagNode *FindRecordNodeEnd(u16 id);

extern u32 FUN_0803ca58(BusyOwner *owner);

/* 0x08059D4C */
u32 FUN_08059d4c(u32 a, u32 id)
{
    FlagNode *node = FindRecordNodeEnd(id);
    BusyOwner *owner;
    u32 bit;

    if (node != 0) {
        owner = node->owner;
        if (owner == 0) goto flags;
        if (FUN_0803ca58(owner) == 0) goto flags;
    }
    return 0;
flags:
    bit = node->flags >> 18;
    bit &= 1;
    return bit;
}
