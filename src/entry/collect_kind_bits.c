/* Collect the chain's kinds as a bitmask — 0x08013A84-0x08013ABB
 *
 * NOT BYTE-MATCHING. 25 of 28 instructions. Everything is reproduced except
 * ONE instruction: the ROM loads the kind byte a second time before the shift
 * and agbcc reuses the first load, which shifts the two branch offsets after it.
 *
 * Walks the chain at +0x104 and sets bit `kind` for every entry whose +0x00
 * kind byte is at most 15; anything above that is skipped, not clamped. The
 * running mask is narrowed to 16 bits after each or, which is what makes it a
 * u16 rather than a word.
 *
 * The kind byte is read TWICE per entry in the ROM, once for the bound and once
 * for the shift. Naming the field twice does not give that -- agbcc merges the
 * two -- and declaring it `vu8` is worse still, because the volatile then also
 * blocks the loop's other optimisations. That one load is what is missing.
 *
 * 260 is `movs r1,#130 / lsls r1,#1`, and the 1 that is shifted lives in a
 * callee-saved register across the whole loop, which is what a local holding it
 * gives (rule 33).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/collect_kind_bits.c
 */

#include "gba_types.h"

#define CHAIN_OFFSET  (130 << 1)    /* 0x104 */
#define KIND_MAX      15

typedef struct KindEntry {
    u8                kind;     /* +0x00 */
    u8                pad01[7];
    struct KindEntry *next;     /* +0x08 */
} KindEntry;

extern u8 gRam02022E50[];

/* 0x08013A84 */
u32 FUN_08013a84(void)
{
    u16 result = 0;
    u8 *base = gRam02022E50;
    KindEntry *entry = *(KindEntry **)(base + CHAIN_OFFSET);
    u32 one;
    u32 bit;

    if (entry != 0) {
        one = 1;
        do {
            if (entry->kind <= KIND_MAX) {
                bit = one;
                bit <<= entry->kind;
                result |= bit;
            }
            entry = entry->next;
        } while (entry != 0);
    }
    return result;
}
