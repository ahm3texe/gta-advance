/* Anchor access and advancement — 0x08050994-0x080509C3
 *
 * Return gRam02030330.base, the SAME field read by GetBase at 0x080509DC.
 * Identical sibling getters such as GetPoolA..D are common in this repository.
 *
 * Keep Anchor identical to gRam02030330_gets.c; conflicting extern types for
 * the same symbol fail make check (TYPES-001).
 *
 * SEPARATE FILE: adding this beside its siblings moves the shared literal
 * pool and breaks existing matches (see src/world/pool_first.c).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/get_slot_unk10.c
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

/* 0x08050994 */
u32 GetBaseAlt(void) { return gRam02030330.base; }

/* 0x080509A0 */
void AdvanceAnchor(void)
{
    if (gRam02030330.unk08 != 1) {
        gRam02030330.unk20 = 2400;
        gRam02030330.unk08 = 3;
        gRam02030330.unk04 = 0;
    } else {
        gRam02030330.unk20 += 120;
    }
}
