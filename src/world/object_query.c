/* Object query — 0x080381F8-0x08038233
 *
 * Copy a constant four-word ROM structure to a local and pass it to the test
 * function; structure assignment produces the ldmia/stmia pair.
 *
 * BYTE-MATCHING. Explicit early return-0 null checks put the zero block
 * before the literal pool, as in the ROM. Equivalent nested if statements
 * moved it after the pool.
 *
 * Two adjacent matching functions are separate: SubmitObject in
 * submit_object.c and GetInnerId in get_inner_id.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/object_query.c
 */

#include "gba_types.h"


typedef struct Params {
    u32 a;
    u32 b;
    u32 c;
    u32 d;
} Params;

#define DEFAULT_PARAMS  (*(const Params *)0x083493D4)

typedef struct Object {
    u8      pad00[8];
    u8      pad08[0x20];
    u32    *target;             /* +0x28 */
} Object;

extern u32    FUN_0804293c(u32 *target, const Params *params);

/* 0x080381F8 */
u32 ProbeObject(Object *object)
{
    Params params;

    params = DEFAULT_PARAMS;

    if (object == 0)
        return 0;
    if (object->target == 0)
        return 0;
    if (FUN_0804293c(object->target, &params) != 0)
        return 1;

    return 0;
}
