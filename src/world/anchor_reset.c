/* Anchor sifirlama — 0x08050918-0x0805096F
 *
 * gRam02030330 struct'inin alanlarini sifirliyor: ilk fonksiyon 7 alan
 * (+4/+8/+12/+16/+24/+28/+32/+40), ikincisi ek 4 (+24 ve +28 sirasi ters,
 * +36/+44/+48/+52). Yani ilki bir alt kume, ikinci tam sifirlama.
 * Ucuncusu (0x08050958) daha da dar bir alt kume: +8/+16/+24/+32/+40.
 * +12 = 40 (varsayilan boyut) ucunde de yaziliyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/anchor_reset.c
 */

#include "gba_types.h"

#define DEFAULT_SIZE   40

typedef struct Anchor {
    u32 unk00;                  /* +0x00 */
    u32 unk04;                  /* +0x04 */
    u32 unk08;                  /* +0x08 */
    u32 unk0C;                  /* +0x0C = boyut */
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
