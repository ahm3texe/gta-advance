/* Clear the counter, then walk the list — 0x08035848-0x08035867
 *
 * The callee's pool word carries the Thumb bit, so the `__thumb`-suffixed
 * symbol is what resolves to the address | 1 (tools/agbcc_build.py).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/reset_then_walk_list.c
 */

#include "gba_types.h"

typedef struct Node Node;
typedef struct List List;

extern u32 gRam02027EFC;
extern u32 gList02027EF0;

extern void ListForEach(List *list, void (*fn)(Node *node));
extern void FUN_08035930__thumb(Node *node);

/* 0x08035848 */
void FUN_08035848(void)
{
    gRam02027EFC = 0;
    ListForEach((List *)&gList02027EF0, FUN_08035930__thumb);
}
