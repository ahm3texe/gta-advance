/* Kimlik icin serbest listeden dugum edinip kaydini baglama
 * 0x08054570-0x08054607  (152 bayt)
 *
 * TASLAK
 */

#include "gba_types.h"

#define SPARE_ID    0x7FEF
#define ENTRY_SIZE  64

typedef struct Node {
    struct Node *next;              /* +0x00 */
    struct Node *prev;              /* +0x04 */
    u16          key;               /* +0x08 kimlik */
    u8           slot;              /* +0x0A */
    u8           kind;              /* +0x0B bayrak bayti */
} Node;

typedef struct Entry {
    u8 pad00[ENTRY_SIZE];
} Entry;

typedef struct RecordTable {
    u8     pad00[4];                /* +0x00 */
    int    count;                   /* +0x04 gecerli kimlik ust siniri */
    u8     pad08[0x14];             /* +0x08..0x1B */
    Entry *entries;                 /* +0x1C */
} RecordTable;

#define RECORD_TABLE  ((const RecordTable *)0x08D49C00)

extern Node *gList02035A80;         /* 0x02035A80 liste basligi */

extern void ListRemove(Node **list, Node *node);
extern void InsertSorted(Node **list, Node *node, s32 id);
extern void FUN_080521c4(Node *node, const Entry *entry);

/* 0x08054570 */
Node *GetOrCreateRecordNode(s32 id)
{
    Node **list;
    Node  *cur;
    Node  *spare;
    Node  *node;
    s32    k;
    s32    flag;

    if (id >= RECORD_TABLE->count)
        goto none;

    list = &gList02035A80;
    cur = list[0];
    spare = list[1];
    goto test;

step:
    cur = cur->next;
test:
    if (cur == 0)
        goto scanned;
    if (cur->key == id)
        goto found;
    if (cur->key <= id)
        goto step;

scanned:
    if (spare->key == SPARE_ID)
        goto insert;
    goto none;

found:
    node = cur;
    goto check;

insert:
    ListRemove(list, spare);
    spare->key = id;
    k = spare->kind & 15;
    spare->slot = 0;
    k = (u8)(k | 2);
    k = k & ~1;
    spare->kind = k;
    InsertSorted(list, spare, id);
    node = spare;

check:
    if (node != 0)
        goto body;

none:
    return 0;

body:
    flag = 2;
    flag &= node->kind;
    if (flag != 0)
        FUN_080521c4(node, RECORD_TABLE->entries + id);

    return node;
}
