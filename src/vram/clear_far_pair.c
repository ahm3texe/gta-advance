/* Clear the +0x1F50 and +0x1F54 words — 0x08011CF4-0x08011D13
 *
 * Both offsets are far past a Thumb immediate, so each comes through the
 * literal pool and is added to the base at run time. The second is NOT derived
 * from the first: the ROM loads 0x1F54 separately rather than adding 4, which
 * is what two independent offsets give (rule 65 would derive it).
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/vram/clear_far_pair.c
 */

#include "gba_types.h"

#define FIRST_OFFSET   0x1F50
#define SECOND_OFFSET  0x1F54

extern u8 gRam0201AEF0[];

/* 0x08011CF4 */
void FUN_08011cf4(void)
{
    u8 *base = gRam0201AEF0;
    u32 offset;

    offset = FIRST_OFFSET;
    *(u32 *)(base + offset) = 0;
    offset = SECOND_OFFSET;
    *(u32 *)(base + offset) = 0;
}
