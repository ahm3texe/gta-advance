/* Sayac tamponundan ayirma — 0x0803385C-0x0803388F
 *
 * AddToCounter (src/world/add_to_counter.c) ile ayni baslik (gRom08CA6A08);
 * burada +0x00 doluluk alani kullaniliyor: baslik + 4 + eski doluluk
 * adresi doner, yeni doluluk (u16) 0xBA8'i asarsa geri alip 0 doner.
 * Isaretci `c + (cur + 4)` seklinde yazilmali (ROM once cur+4 hesapliyor).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/alloc_from_counter.c
 */

#include "gba_types.h"
#define ALLOC_MAX 0x0BA8
typedef struct Counter { u16 alloc; u16 used; } Counter;
extern Counter *gRom08CA6A08;
u8 *AllocFromCounter(u32 amount)
{
    Counter *c; u32 cur; u8 *ptr; u32 n;
    c = gRom08CA6A08;
    cur = c->alloc;
    ptr = (u8 *)c + (cur + 4);
    n = cur + amount;
    c->alloc = n;
    if ((u16)n > ALLOC_MAX) {
        c->alloc = n - amount;
        return 0;
    }
    return ptr;
}
