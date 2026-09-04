/* 12 baytlik paketi gonderme — 0x0803C7D4-0x0803C7EB
 *
 * Cagiranin verdigi 12 bayti gRam0202F360'a kopyalayip FUN_08060db4'u
 * cagiriyor. Iki kural birlikte:
 *   - kural 32: struct atamasi ldmia/stmia ciftini uretiyor
 *   - kural 35: sondaki `pop {r0}; bx r0` donus tipinin void oldugunu
 *     soyluyor (u32 donusunde r0 canli kalirdi)
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/submit_pack.c
 */

#include "gba_types.h"

typedef struct Pack12 {
    u32 a;
    u32 b;
    u32 c;
} Pack12;

extern Pack12 gRam0202F360;

extern void FUN_08060db4(Pack12 *pack, u32 arg);

/* 0x0803C7D4 */
void SubmitPack(Pack12 *src, u32 arg)
{
    gRam0202F360 = *src;
    FUN_08060db4(src, arg);
}
