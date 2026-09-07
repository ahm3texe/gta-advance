/* Sayaca ekleme — 0x08033824-0x0803385B
 *
 * gRom08CA6A08'in gosterdigi basligin +0x02 dolulugunu amount kadar
 * artiriyor; u16 sonucu 3200'u asarsa geri alip 0, aksi halde
 * gRam02027320 + eski doluluk doner.
 *
 * OLCULEN: gRam02027320'nin ADRESI once ayri bir yerele alinmali (kural 37
 * / release_slot.c olcutu); yoksa havuz sirasi ters donuyor ve yukleme
 * yer degistiriyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/add_to_counter.c
 */

#include "gba_types.h"
#define COUNTER_MAX 3200
typedef struct Counter { u16 pad00; u16 used; } Counter;
extern Counter *gRom08CA6A08;
extern u32 gRam02027320;
u32 AddToCounter(u32 amount)
{
    Counter *c; u32 *gp; u32 cur; u32 total; u32 n;
    gp = &gRam02027320;
    c = gRom08CA6A08;
    cur = c->used;
    total = *gp + cur;
    n = cur + amount;
    c->used = n;
    if ((u16)n > COUNTER_MAX) {
        c->used = n - amount;
        return 0;
    }
    return total;
}
