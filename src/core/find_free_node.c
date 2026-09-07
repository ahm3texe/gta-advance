/* Serbest dugum listesinde kimlik arama — 0x08055954-0x0805596F
 *
 * FindNode (src/world/node_search.c, 0x08055AA8) ile KOMUT KOMUT ayni;
 * tek fark liste basi: orada gNodeListHead (0x02035A70), burada
 * gList02035A80. tools/find_twins.py %92.9 benzerlikle isaret etti.
 *
 * Liste kimlige gore SIRALI; arama kimligi gecince 0 donuyor. Baslik
 * nesnesinin +0x00'i listenin ilk dugumu oldugu icin dongu dogrudan
 * baslik adresinden basliyor -- tek `ldr r0,[r0,#0]` hem baslangici
 * hem ilerlemeyi yapiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/find_free_node.c
 */

#include "gba_types.h"

typedef struct Node {
    struct Node *next;          /* +0x00 */
    u8           pad04[4];
    u16          id;            /* +0x08 */
} Node;

extern Node *gList02035A80;

/* 0x08055954 */
Node *FindFreeNode(int id)
{
    Node *node;
    int found;

    node = (Node *)&gList02035A80;
    for (;;) {
        node = node->next;
        if (node == 0)
            return 0;

        found = node->id;
        if (found == id)
            return node;
        if (found > id)
            return 0;
    }
}
