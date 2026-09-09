/* CallWithOffset — 0x080509C4-0x080509DB
 *
 * Three small functions in the cluster: add offset to structure +0x10 and
 * call FUN_080504B4; read +0x10; write +0x2C.
 *
 * BYTE-MATCHING. The ROM's pop {r0}; bx r0 overwrites the callee's r0 result,
 * so the wrapper returns void, not u32. The correct signature reproduces
 * the epilogue and all register allocation.
 *
 * Matching siblings: src/world/gRam02030330_gets.c
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/offset_helpers.c
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

extern u32 FUN_080504b4(u32 addr);

/* 0x080509C4 */
void CallWithOffset(u32 offset)
{
    u32 addr;

    addr = gRam02030330.base + offset;
    FUN_080504b4(addr);
}
