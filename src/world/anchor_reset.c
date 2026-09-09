/* Reset the anchor — 0x08050918-0x0805096F
 *
 * Reset fields in gRam02030330: the first function handles
 * +4/+8/+12/+16/+24/+28/+32/+40. The second adds +36/+44/+48/+52 and reverses
 * the order of +24 and +28. Thus the first resets a subset and the second
 * performs the full reset. The third (0x08050958) resets a smaller subset:
 * +8/+16/+24/+32/+40. All three write +12 = 40 (the default size).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/anchor_reset.c
 */

#include "gba_types.h"

#define DEFAULT_SIZE   40

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

/* 0x08050918 */
void ResetAnchorSmall(void)
{
    gRam02030330.unk0C = DEFAULT_SIZE;
    gRam02030330.unk08 = 0;
    gRam02030330.unk04 = 0;
    gRam02030330.base = 0;
    gRam02030330.unk18 = 0;
    gRam02030330.unk1C = 0;
    gRam02030330.unk20 = 0;
    gRam02030330.unk28 = 0;
}

/* 0x08050934 */
void ResetAnchorFull(void)
{
    gRam02030330.unk0C = DEFAULT_SIZE;
    gRam02030330.unk08 = 0;
    gRam02030330.unk04 = 0;
    gRam02030330.base = 0;
    gRam02030330.unk18 = 0;
    gRam02030330.unk20 = 0;
    gRam02030330.unk1C = 0;
    gRam02030330.unk28 = 0;
    gRam02030330.slot = 0;
    gRam02030330.unk30 = 0;
    gRam02030330.unk34 = 0;
    gRam02030330.unk24 = 0;
}

/* 0x08050958 */
void ResetAnchorPartial(void)
{
    gRam02030330.unk0C = DEFAULT_SIZE;
    gRam02030330.unk08 = 0;
    gRam02030330.base = 0;
    gRam02030330.unk18 = 0;
    gRam02030330.unk20 = 0;
    gRam02030330.unk28 = 0;
}
