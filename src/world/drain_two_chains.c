/* Iki zinciri sonuna kadar yurume — 0x0805151C-0x0805153F
 *
 * Iki getiriciden gelen zinciri bos degilse sonuna kadar takip ediyor;
 * sonuc kullanilmiyor. ROM her ikisinde de ONCE bos kontrolu, sonra
 * do/while yapisi kuruyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/drain_two_chains.c
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
