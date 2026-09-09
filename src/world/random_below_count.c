/* Random index below the kind count — 0x080646AC-0x080646F7
 *
 * Return 0 if gRom08BD3448.slots[kind]->count (+0x10) is zero. Otherwise
 * choose the smallest 2^k-1 mask covering the count and repeatedly draw
 * FUN_0803258c() & mask until it is below the count (unsigned bcs).
 * A separate KindNode view exposes +0x10 through gRom08BD3448.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/random_below_count.c
 */

#include "gba_types.h"
typedef struct KindNode { u8 pad00[16]; u32 count; } KindNode;
typedef struct KindRoot { u32 pad00; KindNode **slots; } KindRoot;
extern KindRoot gRom08BD3448;
extern u32 FUN_0803258c(void);
u32 RandomBelowCount(u16 kind)
{
    u32 n; u32 mask; u32 r;
    n = gRom08BD3448.slots[kind]->count;
    if (n == 0)
        return 0;
    mask = 63;
    if (n <= 32) {
        mask = 31;
        if (n <= 16) {
            mask = 15;
            if (n <= 8) {
                mask = 7;
                if (n <= 4) {
                    mask = 1;
                    if (n > 2)
                        mask = 3;
                }
            }
        }
    }
    do {
        r = FUN_0803258c() & mask;
    } while (r >= n);
    return r;
}
