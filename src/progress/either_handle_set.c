/* Is either handle set — 0x08030DB4-0x08030DD7
 *
 * The counterpart of src/progress/release_two_handles.c over the same two
 * words. It reads +0x1360 first and only looks at +0x1358 when that is null.
 *
 * The ROM DERIVES the second offset from the first (`subs r2,#8`) instead of
 * building it again, which is rule 65's shape: one offset variable assigned
 * twice, not two separate constants.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/either_handle_set.c
 */

#include "gba_types.h"

#define HANDLE_B  (155 << 5)     /* 0x1360 */
#define HANDLE_A  (HANDLE_B - 8) /* 0x1358 */

extern u8 gRam02025810[];

/* 0x08030DB4 */
u32 FUN_08030db4(void)
{
    u8 *base = gRam02025810;
    u32 offset = HANDLE_B;

    if (*(u32 *)(base + offset) != 0) goto yes;
    offset = HANDLE_A;
    if (*(u32 *)(base + offset) == 0) goto no;
yes:
    return 1;
no:
    return 0;
}
