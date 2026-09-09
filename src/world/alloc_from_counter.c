/* Allocate from the counter buffer — 0x0803385C-0x0803388F
 *
 * Uses the same header (gRom08CA6A08) as AddToCounter in add_to_counter.c, but
 * uses the count at +0x00. Return header + 4 + the previous count; if the new
 * u16 count exceeds 0xBA8, roll back and return 0. Write the pointer as
 * `c + (cur + 4)` because the ROM computes cur+4 first.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/alloc_from_counter.c
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
