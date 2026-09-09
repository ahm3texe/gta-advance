/* Script command: call unless flag 0x80000 is set — 0x0805A1F4-0x0805A219
 *
 * Reads bit 19 of the +0x18 word of the record gRam020272C8 points at, and
 * answers 0 without calling anything when it is set.
 *
 * Rule 71: the call is the arm that falls through in the ROM, so it is the
 * `then` arm and the test is written `== 0`.
 *
 * gRam020272C8 is declared `u32` here because that is the type its other users
 * give it (src/core/nodelist_c6.c and the sibling node-list files, which store
 * a node pointer into it); the consistency check requires one type per symbol.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_call_unless_flag.c
 */

#include "gba_types.h"

#define BLOCKED  (128 << 12)

typedef struct FlagRecord {
    u8  pad00[0x18];
    u32 flags;                  /* +0x18 */
} FlagRecord;

extern u32 gRam020272C8;

extern u32 FUN_08057a04(u32 a);

/* 0x0805A1F4 */
u32 FUN_0805a1f4(u32 a)
{
    FlagRecord *record = (FlagRecord *)gRam020272C8;
    u32 result;

    if ((record->flags & BLOCKED) == 0)
        result = FUN_08057a04(a);
    else
        result = 0;
    return result;
}
