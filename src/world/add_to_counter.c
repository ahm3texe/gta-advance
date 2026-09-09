/* Add to the counter — 0x08033824-0x0803385B
 *
 * Increase the used count at +0x02 in the header pointed to by gRom08CA6A08
 * by amount. If the u16 result exceeds 3200, roll back and return 0; otherwise
 * return gRam02027320 + the previous count.
 *
 * MEASURED: first capture the ADDRESS of gRam02027320 in a separate local
 * (rule 37 / release_slot.c). Otherwise the literal-pool order reverses and
 * the load moves.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/add_to_counter.c
 */

#include "gba_types.h"
#define COUNTER_MAX 3200
typedef struct Counter { u16 alloc; u16 used; } Counter;
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
