/* Drop the object list head — 0x08014FAC-0x08014FB7
 *
 * Byte for byte the same body as src/glue/clear_object_list.c at 0x08014FB8,
 * twelve bytes earlier in the ROM. Two identical functions, not one with a
 * wrong boundary: both are twelve bytes and both end in `bx lr`.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/clear_object_list_alt.c
 */

#include "gba_types.h"

typedef struct Obj Obj;

extern Obj *gRam020230A0;

/* 0x08014FAC */
void FUN_08014fac(void)
{
    gRam020230A0 = 0;
}
