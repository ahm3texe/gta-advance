/* Anchor erisim ve ilerletme — 0x08050994-0x080509C3
 *
 * gRam02030330.base alanini dondurur; 0x080509DC'deki GetBase ile AYNI
 * alani okuyan ikinci bir erisimci (bu depoda GetPoolA..D gibi ozdes
 * kardes getiriciler yaygin).
 *
 * Anchor tanimi src/world/gRam02030330_gets.c ile birebir ayni tutulmali;
 * ayni sembole celiskili extern turu vermek `make check`i kirar (TYPES-001).
 *
 * NEDEN AYRI DOSYA: kardeslerinin yanina eklemek ortak literal havuzunu
 * kaydirip zaten eslesen fonksiyonlari bozuyor (bkz. src/world/pool_first.c).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/get_slot_unk10.c
 */

#include "gba_types.h"

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
