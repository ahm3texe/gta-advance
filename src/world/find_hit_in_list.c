/* Isabet listesinde arama — 0x08041F8C-0x08042009 (126 bayt)
 *
 * DURUM: 41/61 komut, YAKIN ISKA (eslesmiyor). Boyut tutuyor.
 *
 * Listedeki her giris icin +0x06 bayraklarinin alt iki biti silinip
 * (x,y) ile FUN_08040564 cagriliyor; sonuc varsa +0x08 turunde 10
 * maskesi kuruluysa bayraklara 2 (ve tur bit1 kuruluysa 0x20), degilse 1
 * eklenip sonuc donuyor. Bulunmazsa fallback doner.
 *
 * KALAN FARK: ROM `orrs r1,r7` ile bayraklara r7 = 0 degerini de OR'luyor
 * (dongu on-basliginda `movs r7,#0`, callee-saved). Yani kaynakta sifir
 * degerli, dongu boyunca yasayan bir degisken var. `extra = 0` yerelini
 * dongu icinde (41), dongu disinda (41/66), maskeyle birlikte hoist
 * (39) yazmak ROM'un yazmac dagitimini vermedi; `(count & 0)` hilesi 47
 * veriyor ama kaynak olarak savunulamaz. Yazmac dagitiminda ctx r8,
 * fallback r9 (bende r7/r8) -- ekstra bir degisken daha olmali.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/find_hit_in_list.c
 */

#include "gba_types.h"
typedef struct Hit { u16 x; u16 y; u8 pad04[2]; u16 flags; } Hit;
typedef struct Res { u8 pad00[8]; u8 kind; } Res;
extern Res *FUN_08040564(u32 ctx, u32 x, u32 y);
Res *FindHitInList(u32 ctx, Hit *list, s32 count, Res *fallback)
{
    s32 i; u32 extra; u32 m; u32 v; Res *r;
    for (i = 0; i < count; i++) {
        extra = 0;
        m = 0xFFFC;
        m = m & list->flags;
        list->flags = m;
        r = FUN_08040564(ctx, list->x, list->y);
        if (r != 0) {
            if (r->kind & 10) {
                v = list->flags | 2 | extra;
                list->flags = v;
                if (r->kind & 2) {
                    v |= 0x20;
                    list->flags = v;
                }
            } else {
                list->flags = 1 | list->flags;
            }
            return r;
        }
        list++;
    }
    return fallback;
}
