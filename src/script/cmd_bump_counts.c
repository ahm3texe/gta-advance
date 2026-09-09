/* Script commands: bump a counter and succeed
 * 0x0805B11C-0x0805B127 and 0x0805B128-0x0805B133
 *
 * Two handlers with the same body and different callees. They take no operand
 * and always answer 1. The two siblings of the same shape at 0x0805B908 and
 * 0x0805B9F8 are in their own files: a source file is compiled as one blob and
 * compared against one ROM range, so only functions that are ADJACENT in the
 * ROM can share a file.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_bump_counts.c
 */

#include "gba_types.h"

extern void BumpCount70(void);
extern void BumpCount80(void);

/* 0x0805B11C */
u32 FUN_0805b11c(void)
{
    BumpCount70();
    return 1;
}

/* 0x0805B128 */
u32 FUN_0805b128(void)
{
    BumpCount80();
    return 1;
}
