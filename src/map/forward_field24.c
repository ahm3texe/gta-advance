/* Hand the +0x24 field on — 0x08041EE4-0x08041EEF
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/map/forward_field24.c
 */

#include "gba_types.h"

typedef struct FieldOwner {
    u8  pad00[0x24];
    u32 field;                  /* +0x24 */
} FieldOwner;

extern void FUN_0803f7d4(u32 value);

/* 0x08041EE4 */
void FUN_08041ee4(FieldOwner *owner)
{
    FUN_0803f7d4(owner->field);
}
