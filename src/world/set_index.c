/* Set the table index — 0x08008094-0x080080A1
 *
 * Pass argument two to SetLanguage and return 1; argument one is unused.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/set_index.c
 */

#include "gba_types.h"

extern void SetLanguage(u32 index);

/* 0x08008094 */
u32 SetIndexReturnOne(u32 unused, u32 index)
{
    SetLanguage(index);

    return 1;
}
