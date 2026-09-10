/* Store the masked value at +0x10 and +0x0C — 0x0801D86C-0x0801D883
 *
 * 0x03FFFFFF comes through the literal pool, and rule 33 applies: the mask is
 * materialised first and the argument anded INTO it, so one register carries
 * the result to both stores.
 *
 * The +0x10 store comes BEFORE the +0x0C one; the field writes are not in
 * offset order.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/set_masked_pair_and_call.c
 */

#include "gba_types.h"

#define VALUE_MASK  0x03FFFFFF

typedef struct MaskedPair {
    u8  pad00[0x0C];
    u32 second;                 /* +0x0C */
    u32 first;                  /* +0x10 */
} MaskedPair;

extern void FUN_0801a560(void *obj);

/* 0x0801D86C */
void FUN_0801d86c(MaskedPair *pair, u32 value)
{
    u32 masked = VALUE_MASK;

    masked &= value;
    pair->first = masked;
    pair->second = masked;
    FUN_0801a560(pair);
}
