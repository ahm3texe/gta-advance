/* Index of the model with this id — 0x08038680-0x080386A5
 *
 * Walks the 120-entry model table at 0x08CA7E64 and answers the index of the
 * first entry whose +0x04 id matches, or -1.
 *
 * The stride is the 80 bytes of Phase1Model, and the count comes from the ROM's
 * `cmp r1,#119 / bls`: an UNSIGNED bound, unlike the signed ones in
 * src/progress/, so the index is unsigned here.
 *
 * The walking pointer starts at the table base plus 4, and the base goes
 * through a local of its own so that it does. Applied to the symbol directly
 * the 4 folds into the pool constant and the `adds r2,r0,#4` disappears; the
 * ROM keeps 0x08CA7E64 in the pool and adds the field offset at run time
 * (rule 65).
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entity/find_model_by_id.c
 */

#include "gba_types.h"
#include "phase1_types.h"

#define MODEL_COUNT   120

extern const Phase1Model gRom08CA7E64[];

/* 0x08038680 */
s32 FUN_08038680(u32 id)
{
    u32 index = 0;
    const u8 *base = (const u8 *)gRom08CA7E64;
    const u8 *field = base + 4;

    while (index <= MODEL_COUNT - 1) {
        if (*(const u32 *)field == id)
            return index;
        field += sizeof(Phase1Model);
        index++;
    }
    return -1;
}
