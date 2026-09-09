/* Read one stat word by index — 0x0805B214-0x0805B223
 *
 * Not a script handler: it has no prologue at all and returns through `bx lr`,
 * and it is a call target rather than a table entry. Sits between the handlers
 * only because of where the linker put it.
 *
 * gRam02035AD0 is recorded in data/ram_map.csv with an unknown extent; this
 * function indexes it without a bound, so it does not settle one either.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/read_stat_word.c
 */

#include "gba_types.h"

extern u32 gRam02035AD0[];

/* 0x0805B214 */
u32 FUN_0805b214(u32 index)
{
    return gRam02035AD0[index];
}
