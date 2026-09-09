/* Run FUN_08037B50 over the whole list — 0x08038584-0x0803859B
 *
 * The list header is reached as an address rather than a typed object, because
 * src/misc/state_getters.c already declares gUnk0202F2C0 as a `u32` and the
 * consistency check requires one extern type per symbol.
 *
 * The callee's pool word has bit 0 set: it is a Thumb function pointer. The
 * `__thumb`-suffixed symbol is what resolves to the address | 1
 * (tools/agbcc_build.py); the plain name would give an even address and the
 * call through it would switch the core to ARM.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entity/step_all_entities.c
 */

#include "gba_types.h"

typedef struct ListNode ListNode;
typedef struct List List;

extern u32 gUnk0202F2C0;

extern void ListForEach(List *list, void (*fn)(ListNode *node));
extern void FUN_08037b50__thumb(ListNode *node);

/* 0x08038584 */
void FUN_08038584(void)
{
    ListForEach((List *)&gUnk0202F2C0, FUN_08037b50__thumb);
}
