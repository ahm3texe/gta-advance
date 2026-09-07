/* Oturum ozniteligini kurma — 0x08031388-0x080313DF
 *
 * gRam02026DA0 blogunu sifirlayip gRom083444E8 tablosundan 512/32/32 ile
 * dolduruyor, gRom08CA635C'yi bagliyor, +8/+10'a 200x8 yazip bitiriyor.
 *
 * OLCULEN: `if (attr != 0)` ROM'da GERCEKTEN var (cmp r4,#0). Adres
 * `((u8 *)0x02026DA0)` cast makrosuyla verilirse agbcc karsilastirmayi
 * katliyor; `extern u8 gRam02026DA0[]` dizi sembolu olarak verilince
 * kaliyor. Kaynakta muhtemelen bir isaretci donduren yardimci vardi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/init_session_attr.c
 */

#include "gba_types.h"
extern u8 gRam02026DA0[];
#define SESSION_ATTR   gRam02026DA0
#define ATTR_W  200
#define ATTR_H  8
extern const u8 gRom083444E8[];
extern const u8 gRom08CA635C[];
extern void FUN_08014ffc(u8 *dest, u32 count, void *src, u32 *value);
extern void FUN_08014040(u8 *dest, const u8 *src, u32 a, u32 b, u32 c, u32 d);
extern void FUN_08014ee4(u8 *dest, const u8 *value);
extern void FUN_08015038(u8 *dest);
void InitSessionAttr(void)
{
    u8 *attr;
    attr = SESSION_ATTR;
    FUN_08014ffc(attr, 0, 0, 0);
    FUN_08014040(attr, gRom083444E8, 512, 32, 32, 0);
    FUN_08014ee4(attr, gRom08CA635C);
    if (attr != 0) {
        *(u16 *)(attr + 8) = ATTR_W;
        *(u16 *)(attr + 10) = ATTR_H;
    }
    FUN_08015038(attr);
}
