/* Ikinci +0x10 getirici — 0x08050994-0x0805099F
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
    u8  pad00[0x10];
    u32 base;                   /* +0x10 */
    u8  pad14[0x18];
    u32 slot;                   /* +0x2C */
} Anchor;

extern Anchor gRam02030330;

/* 0x08050994 */
u32 GetBaseAlt(void) { return gRam02030330.base; }
