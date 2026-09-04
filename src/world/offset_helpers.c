/* CallWithOffset — 0x080509C4-0x080509DB
 *
 * Uc kucuk fonksiyon. Struct'in +0x10'undaki bir tabana offset ekleyip
 * FUN_080504B4 cagirici; +0x10 okuyucu; +0x2C yazici.
 *
 * BYTE-MATCHING. ROM `pop {r0}; bx r0` ile cagrilan fonksiyonun r0
 * sonucunu eziyor; bu sarmalayicinin donus tipi u32 degil void. Dogru
 * imza epilogu ve tum register dagitimini birebir uretiyor.
 *
 * Eslesen kardesler: src/world/gRam02030330_gets.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/offset_helpers.c
 */

#include "gba_types.h"

typedef struct Anchor {
    u8  pad00[0x10];
    u32 base;                   /* +0x10 */
    u8  pad14[0x18];
    u32 slot;                   /* +0x2C */
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
