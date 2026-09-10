/* Two anchor getters — 0x08050970-0x0805097B and 0x0805097C-0x08050987
 *
 * Neither was in data/functions.csv: they filled the 24-byte gap between
 * ResetAnchorPartial and GetAnchorUnk04, and they have exactly the shape of the
 * getters on either side of them, over the +0x08 and +0x0C fields.
 *
 * The Anchor body is copied from src/world/offset_helpers.c, which must keep
 * the same layout: the consistency check requires one struct body per shared
 * symbol.
 *
 * No prologue: nothing is called and both return through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/anchor_gets_08_0c.c
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

/* 0x08050970 */
u32 FUN_08050970(void)
{
    return gRam02030330.unk08;
}

/* 0x0805097C */
u32 FUN_0805097c(void)
{
    return gRam02030330.unk0C;
}
