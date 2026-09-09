/* Anchor +0x04 getter — 0x08050988-0x08050993
 *
 * PREVIOUSLY UNMAPPED. A verified forwarding wrapper's actual bl targets this
 * address, proving the function entry. As a leaf without a prologue, it was
 * missed by prologue-based discovery.
 *
 * Keep Anchor IDENTICAL to src/world/anchor_reset.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/get_anchor_unk04.c
 */

#include "gba_types.h"

typedef struct Anchor {
    u32 unk00;                  /* +0x00 */
    u32 unk04;                  /* +0x04 */
    u32 unk08;                  /* +0x08 */
    u32 unk0C;                  /* +0x0C = size */
    u32 base;                   /* +0x10 */
    u32 unk14;                  /* +0x14 */
    u32 unk18;                  /* +0x18 */
    u32 unk1C;                  /* +0x1C */
    u32 unk20;                  /* +0x20 */
    u32 unk24;                  /* +0x24 */
    u32 unk28;                  /* +0x28 */
    u32 slot;                   /* +0x2C */
    u32 unk30;                  /* +0x30 */
    u32 unk34;                  /* +0x34 */
    u32 unk38;                  /* +0x38 */
} Anchor;

extern Anchor gRam02030330;

/* 0x08050988 */
u32 GetAnchorUnk04(void) { return gRam02030330.unk04; }
