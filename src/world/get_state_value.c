/* Durum degeri tablosu — 0x08024044-0x0802418B (328 bayt)
 *
 * DURUM: 92/162 komut, YAKIN ISKA (eslesmiyor). Boyut tutuyor.
 *
 * Nesnenin +0x64 durumuna gore gRom08CA6138 (alt != 0 ise gRom08CA617C)
 * tablosundan bir u32 secip <<16 doner. 51 -> [9], 76 -> [8], 101 ->
 * +0x84 alt nesnesinin +0x09 turu 35 ise [15] degilse [10], 34 -> +0x90
 * evresine gore 35 girisli atlama tablosu (12..46): 12/38/39 -> [7],
 * 22 -> 101 ile ayni secim, 28 -> +0x8C > 0x3FFFF ise 0 degilse [4],
 * 36 -> 0, 46 -> [8]; diger her sey (18 dahil) FUN_0804fb3c(alt)
 * sonucunun u16'siyla indisler.
 *
 * KALAN FARK, TEK MEKANIZMA: agbcc bende ozdes govdeleri BIRLESTIRIYOR
 * (dis case 76 `[8]` ile ic case 46 `[8]`, dis 101 ile ic 22, ic default
 * ile dis default) -- ROM'da bunlar AYRI bloklar (0x08024086/0x08024172,
 * 0x0802408A/0x0802415C). Oysa ayni ROM iki `return 0`u birlestirmis
 * (0x08024150). Denenen: her case'e kendi `return x << 16` (85), ic
 * case'leri ROM govde sirasina dizmek (91), ic default'u goto ile dis
 * default'a baglamak (79). Birlestirmeyi engelleyen kaynak bicimi
 * bulunamadi; muhtemelen ic switch ayri bir deyim/yardimci.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/get_state_value.c
 */

#include "gba_types.h"
#define VAL_LIMIT   0x3FFFF
#define KIND_35     35
typedef struct Sub { u8 pad00[9]; u8 kind; } Sub;
typedef struct Obj { u8 pad00[0x64]; u8 state; u8 pad65[0x1F]; Sub *sub; u8 pad88[4]; s32 amount; s32 phase; } Obj;
extern const u32 gRom08CA6138[];
extern const u32 gRom08CA617C[];
extern u32 FUN_0804fb3c(Sub *sub);
u32 GetStateValue(Obj *obj, u32 alt)
{
    const u32 *tbl; Sub *sub; u32 v;
    sub = obj->sub;
    tbl = gRom08CA6138;
    if (alt != 0)
        tbl = gRom08CA617C;
    switch (obj->state) {
    case 51:
        v = tbl[9];
        break;
    case 76:
        v = tbl[8];
        break;
    case 101:
        if (sub != 0 && sub->kind == KIND_35)
            v = tbl[15];
        else
            v = tbl[10];
        break;
    case 34:
        switch (obj->phase) {
        case 12:
        case 38:
        case 39:
            v = tbl[7];
            break;
        case 22:
            if (sub != 0 && sub->kind == KIND_35)
                v = tbl[15];
            else
                v = tbl[10];
            break;
        case 28:
            if (obj->amount > VAL_LIMIT)
                return 0;
            v = tbl[4];
            break;
        case 36:
            return 0;
        case 46:
            v = tbl[8];
            break;
        default:
            v = tbl[(u16)FUN_0804fb3c(sub)];
            break;
        }
        break;
    case 18:
    default:
        v = tbl[(u16)FUN_0804fb3c(sub)];
        break;
    }
    return v << 16;
}
