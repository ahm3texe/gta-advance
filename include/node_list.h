/* Shared layout for the linked list whose head is stored at 0x02035780.
 *
 * Two source files previously declared this symbol with conflicting types
 * (NodeC4 * and u32). Its use at 0x08053D48 resolves the ambiguity: traversal
 * follows `node = node->next`, identifying the symbol as a list-head pointer.
 * Keep the layout here rather than duplicating it across source files.
 */
#ifndef GUARD_NODE_LIST_H
#define GUARD_NODE_LIST_H

#include "gba_types.h"

typedef struct NodeC4 {
    struct NodeC4 *next;        /* +0x00 */
    u8  pad04[4];
    u16 id;                     /* +0x08 */
    u8  pad0A;
    u8  unk0B_lo : 4;           /* +0x0B bits 0-3 */
    s8  kind     : 4;           /* +0x0B bits 4-7, signed */
} NodeC4;

extern NodeC4 *gRam02035780;    /* list head */

#endif /* GUARD_NODE_LIST_H */
