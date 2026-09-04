/* Isleyici ve paket kurma — 0x0803F67C-0x0803F69B
 *
 * Isleyici isaretcisini +0x08'e yaziyor, cagiranin 12 baytini +0x20'ye
 * kopyaliyor, +0x1C'ye degeri koyuyor ve +0x81'deki bayti sifirliyor.
 *
 * Uc kural birlikte:
 *   - kural 32: struct atamasi ldmia/stmia cifti uretiyor
 *   - kural 37: ROM hedef adresi `adds r3,r0,#32` ile AYRI register'a
 *     aliyor; kaynakta da ayri bir isaretci yereli gerekiyor
 *   - kural 35: `pop {r0}; bx r0` -> donus tipi void
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/init_handler_pack.c
 */

#include "gba_types.h"


typedef struct Pack12 {
    u32 a;
    u32 b;
    u32 c;
} Pack12;

typedef struct Slot {
    u8     pad00[8];
    void  *handler;             /* +0x08 */
    u8     pad0C[16];
    u32    value;               /* +0x1C */
    Pack12 pack;                /* +0x20 */
    u8     pad2C[85];
    u8     ready;               /* +0x81 */
} Slot;

/* Saklanan fonksiyon isaretcisinde THUMB BITI (bit 0) kurulu olmali.
   `__thumb` sonekli sembol, adresi | 1 olarak cozumlenir
   (tools/agbcc_build.py). `bl` hedefinde bit eklemek dal ofsetini
   bozacagi icin ayri sembol kullaniliyor. */
extern u8 FUN_0803d13c__thumb[];

/* 0x0803F67C */
void InitHandlerPack(Slot *slot, Pack12 *src, u32 value)
{
    Pack12 *dest;

    slot->handler = FUN_0803d13c__thumb;
    dest = &slot->pack;
    *dest = *src;
    slot->value = value;
    slot->ready = 0;
}
