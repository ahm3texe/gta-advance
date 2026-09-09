/* Increment the counter or trigger — 0x08050108-0x0805012F
 *
 * Test the sub-object's byte at +0xA5 as SIGNED. If positive, call FUN_08055674
 * and return 1; otherwise increment the byte and return 0.
 *
 * The ROM reads the same byte TWICE: ldrb for the increment, then ldrsb for
 * the signed test. The source also uses two reads because different types
 * are needed.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a return value; the return type is u32.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/bump_or_trigger.c
 */

#include "gba_types.h"

#define COUNTER_OFFSET 0xA5

typedef struct Obj {
    u8  pad00[28];
    u8 *sub;                    /* +0x1C */
} Obj;

extern void FUN_08055674(Obj *obj);

/* 0x08050108 */
u32 BumpOrTrigger(Obj *obj)
{
    u8 *counter;
    u8  value;

    counter = obj->sub + COUNTER_OFFSET;
    value = *counter;
    if (*(s8 *)counter > 0) {
        FUN_08055674(obj);
        return 1;
    }
    *counter = value + 1;
    return 0;
}
