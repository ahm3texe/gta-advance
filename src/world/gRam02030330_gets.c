/* gRam02030330 field access — 0x080509DC-0x080509ED
 *
 * Getter for base (+0x10) and setter for slot (+0x2C). CallWithOffset from
 * the same cluster is in a separate file and is byte-matching.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/gRam02030330_gets.c
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

/* 0x080509DC */
u32 GetBase(void)
{
    return gRam02030330.base;
}

/* 0x080509E8 */
void SetSlot(u32 value)
{
    gRam02030330.slot = value;
}
