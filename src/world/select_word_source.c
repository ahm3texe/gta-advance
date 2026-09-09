/* Select a source and read a halfword — 0x0803C45C-0x0803C47B
 *
 * Return the EWRAM halfword for mode 2, the IWRAM halfword for mode 1, or
 * 0 otherwise. Both ROM branches share one ldrh, so the source reads
 * through a shared local pointer.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/select_word_source.c
 */

#include "gba_types.h"

#define SRC_EWRAM ((u16 *)0x020003EC)
#define SRC_IWRAM ((u16 *)0x0300009C)

/* 0x0803C45C */
u32 SelectWordSource(u32 mode)
{
    u16 *src;

    if (mode == 2) {
        src = SRC_EWRAM;
    } else {
        if (mode != 1)
            return 0;
        src = SRC_IWRAM;
    }
    return *src;
}
