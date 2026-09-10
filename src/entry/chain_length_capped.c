/* How long the +0x380 chain is, capped at 32 — 0x080134FC-0x0801351F
 *
 * The chain hangs off +0x380 of the node pool and is followed through each
 * node's +0x14 field. The count stops at 32, so a cycle cannot hang it.
 *
 * The bound is a SIGNED comparison (`cmp r1,#31 / ble`), and the count is
 * returned as it stands: a chain of exactly 32 and a longer one both answer 32.
 *
 * All three exits share ONE return, reached by branching to it: the null test
 * branches there, the in-loop null test branches there, and the bound falls
 * through to it. An early `return count;` for the first test makes agbcc
 * materialise a second zero and costs four instructions.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/chain_length_capped.c
 */

#include "gba_types.h"
#include "phase1_types.h"

#define CHAIN_OFFSET  (224 << 2)    /* 0x380 */
#define CHAIN_MAX     32

typedef struct ChainLink {
    u8                pad00[0x14];
    struct ChainLink *next;     /* +0x14 */
} ChainLink;

extern Phase1NodePool gRam02022AC0;

/* 0x080134FC */
s32 FUN_080134fc(void)
{
    u8 *base = (u8 *)&gRam02022AC0;
    ChainLink *link = *(ChainLink **)(base + CHAIN_OFFSET);
    s32 count = 0;

    if (link != 0) {
        do {
            link = link->next;
            count++;
            if (link == 0)
                goto done;
        } while (count <= CHAIN_MAX - 1);
    }
done:
    return count;
}
