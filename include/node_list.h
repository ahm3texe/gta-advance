/* 0x02035780'deki dugum listesinin ortak yerlesimi.
 *
 * Iki dosya bu sembole dokunuyordu ve CELISKILI tip bildirmislerdi
 * (NodeC4* ve u32).  Karar kullanimdan verildi: 0x08053D48 onu
 * `node = node->next` ile YURUTUYOR, yani liste basi isaretcisi.
 * Govdeyi kopyalamak yerine burada tek yerde tutuluyor.
 */
#ifndef GUARD_NODE_LIST_H
#define GUARD_NODE_LIST_H

#include "gba_types.h"

typedef struct NodeC4 {
    struct NodeC4 *next;        /* +0x00 */
    u8  pad04[4];
    u16 id;                     /* +0x08 */
    u8  pad0A;
    u8  unk0B_lo : 4;           /* +0x0B bit 0-3 */
    s8  kind     : 4;           /* +0x0B bit 4-7, ISARETLI */
} NodeC4;

extern NodeC4 *gRam02035780;    /* liste basi */

#endif /* GUARD_NODE_LIST_H */
