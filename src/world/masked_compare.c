/* Maskeli karsilastirma — 0x08023A80-0x08023A9B
 *
 * 0x02000224'teki sozu ve nesnenin +0x0C alanini ayni maskeyle suzup
 * FARKLIYSA 1, ayniysa 0 donduruyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/masked_compare.c
 */

#include "gba_types.h"

typedef struct MaskedNode {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} MaskedNode;

extern u32 gRam02000224;

/* 0x08023A80 */
u32 MaskedDiffers(MaskedNode *node, u32 mask)
{
    u32 current;
    u32 wanted;

    current = gRam02000224 & mask;
    wanted = node->flags & mask;
    if (current == wanted)
        return 0;
    return 1;
}
