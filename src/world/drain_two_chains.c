/* Walk two chains to their ends — 0x0805151C-0x0805153F
 *
 * Follow each chain returned by two getters to its end if nonempty; the result
 * is unused. In both cases the ROM checks for null FIRST, then uses do/while.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/drain_two_chains.c
 */

#include "gba_types.h"

typedef struct Link {
    struct Link *next;          /* +0x00 */
} Link;

extern Link *GetUnk0202F2C0(void);
extern Link *GetUnk0202F310(void);

/* 0x0805151C */
void DrainTwoChains(void)
{
    Link *node;

    node = GetUnk0202F2C0();
    if (node != 0) {
        do {
            node = node->next;
        } while (node != 0);
    }

    node = GetUnk0202F310();
    if (node != 0) {
        do {
            node = node->next;
        } while (node != 0);
    }
}
