/* Anchor +0x38 <= 15 sorgusu — 0x08050A34-0x08050A47
 *
 * Karsilastirma SIGNED (`ble`), bu yuzden alan s32 olarak okunuyor.
 * tools/find_predicates.py ile bulundu.
 *
 * Anchor tanimi src/world/anchor_reset.c ile BIREBIR AYNI tutulmali.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/is_anchor_small.c
 */

#include "gba_types.h"

#define SMALL_LIMIT 15

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
    u32 unk38;                  /* +0x38 */
} Anchor;

extern Anchor gRam02030330;

/* 0x08050A34 */
u32 IsAnchorSmall(void)
{
    if ((s32)gRam02030330.unk38 <= SMALL_LIMIT)
        return 1;
    return 0;
}
