/* Notify with the +0x1128 index, one less — 0x08031034-0x08031053
 *
 * The word at +0x1128 is a one-based index: zero means "nothing", and the
 * callee is given the value minus one. 0x1128 comes through the literal pool.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/notify_index_1128.c
 */

#include "gba_types.h"

#define INDEX_OFFSET  0x1128

extern u8 gRam02025810[];

extern void FUN_0802eb50(u32 index);

/* 0x08031034 */
void FUN_08031034(void)
{
    u8 *base = gRam02025810;
    u32 index = *(u32 *)(base + INDEX_OFFSET);

    if (index != 0)
        FUN_0802eb50(index - 1);
}
