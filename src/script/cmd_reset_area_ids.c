/* Script command: reset the area ids — 0x0805B9F8-0x0805BA03
 *
 * Same shape as the pair in cmd_bump_counts.c, but not adjacent to them in the
 * ROM, so it needs its own file.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_reset_area_ids.c
 */

#include "gba_types.h"

extern void ResetAreaIds(void);

/* 0x0805B9F8 */
u32 FUN_0805b9f8(void)
{
    ResetAreaIds();
    return 1;
}
