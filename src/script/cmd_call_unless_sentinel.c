/* Script command: call unless the lookup answers 0x7FFF — 0x0805B6C4-0x0805B6F3
 *
 * NOT BYTE-MATCHING. 14 of 22 instructions. Everything except the comparison is
 * reproduced; the behaviour below is what the ROM does.
 *
 * FUN_08055720's answer is narrowed to 16 bits and compared against 0x7FFF, the
 * sentinel meaning "nothing", in which case the handler answers 1 without
 * calling anything.
 *
 * THE WALL. The ROM narrows into a register of its own and then compares
 * against the pool constant:
 *
 *     lsls r0,r0,#16 / lsrs r1,r0,#16 / ldr r0,=0x00007FFF / cmp r1,r0
 *
 * agbcc will not emit that. It folds the shift pair into the constant and
 * compares against 0x7FFF0000 with the value still shifted left, which costs
 * the `lsrs` and swaps the two registers. Six spellings were measured and all
 * six fold the same way:
 *
 *   u16 entry = f(...); if (entry != 0x7FFF)
 *   u16 entry = f(...); u16 none = 0x7FFF; if (entry != none)
 *   u32 raw = f(...); u16 entry = raw; if (entry != 0x7FFF)
 *   u16 entry = f(...); u32 none; none = 0x7FFF; if (entry != none)
 *   u32 entry = f(...) << 16; if ((entry >> 16) != 0x7FFF)
 *   entry = raw << 16; entry >>= 16; if (entry != none)
 *
 * The fold happens in the combiner, before register allocation, so the rule 33
 * and rule 70 levers do not reach it. This is the rule 44 class: a difference
 * with no known source lever. Left here as a record of the behaviour and of
 * what has already been tried, rather than removed.
 *
 * Rule 71 IS reproduced: the call is the arm that falls through, so it is the
 * `then` arm and the test is written against the sentinel with `!=`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_call_unless_sentinel.c
 */

#include "gba_types.h"

#define NO_ENTRY  0x7FFF

extern u32 gRam02035B44;

extern u32 FUN_08055720(u32 value);
extern u32 FUN_08059244(u32 a);

/* 0x0805B6C4 */
u32 FUN_0805b6c4(u32 a)
{
    u16 entry = FUN_08055720(gRam02035B44);
    u32 result;

    if (entry != NO_ENTRY)
        result = FUN_08059244(a);
    else
        result = 1;
    return result;
}
