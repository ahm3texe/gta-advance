/* Empty the object list — 0x08014FB8-0x08014FC3
 *
 * gRam020230A0 is the head of the doubly linked object list
 * (data/ram_map.csv); this drops it without walking the chain.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/clear_object_list.c
 */

#include "gba_types.h"

typedef struct Obj Obj;

extern Obj *gRam020230A0;

/* 0x08014FB8 */
void FUN_08014fb8(void)
{
    gRam020230A0 = 0;
}
