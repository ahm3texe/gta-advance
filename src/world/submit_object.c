/* Submit an object — 0x08038234-0x0803824B
 *
 * Pass FUN_08038608's result, constant 1 and the caller's argument to
 * FUN_08036cac. The epilogue pop {r1}; bx r1 leaves r0 live, so this
 * function RETURNS A VALUE.
 *
 * Siblings: object_query.c (non-matching when this note was written) and
 * object_value.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/submit_object.c
 */

#include "gba_types.h"

typedef struct Object Object;

extern u32 FUN_08038608(Object *object);
extern u32 FUN_08036cac(u32 handle, int one, u32 arg);

/* 0x08038234 */
u32 SubmitObject(Object *object, u32 arg)
{
    return FUN_08036cac(FUN_08038608(object), 1, arg);
}
