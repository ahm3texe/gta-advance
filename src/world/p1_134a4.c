/* Build the list of 31 free doubly linked nodes — 0x080134A4.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x080134A4.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern Phase1NodePool gRam02022AC0;
void FUN_080134a4(void)
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
