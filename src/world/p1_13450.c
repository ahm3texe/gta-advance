/* Cift bagli 31 bos dugumun listesini kur — 0x08013450.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x08013450.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern Phase1NodePool gRam02022AC0;
void FUN_08013450(void)
{
    Phase1ListNode *node;
    s32 id;
    gRam02022AC0.free = &gRam02022AC0.nodes[1];
    gRam02022AC0.used = 0;
    node = &gRam02022AC0.nodes[1];
    node->next = node + 1;
    node->previous = 0;
    node->id = 1;
    node++;
    for (id = 2; id <= 30; id++, node++) {
        node->previous = node - 1;
        node->next = node + 1;
        node->id = id;
    }
    node->previous = node - 1;
    node->next = 0;
    node->id = id;
}
