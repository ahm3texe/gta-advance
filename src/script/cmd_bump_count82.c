/* Script command: bump the 0x08067184 counter — 0x0805B908-0x0805B913
 *
 * Same shape as the pair in cmd_bump_counts.c, but not adjacent to them in the
 * ROM, so it needs its own file.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_bump_count82.c
 */

#include "gba_types.h"

extern void BumpCount82(void);

/* 0x0805B908 */
u32 FUN_0805b908(void)
{
    BumpCount82();
    return 1;
}
