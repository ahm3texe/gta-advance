/* Pass the object and a field three links down — 0x080500F8-0x080500FF+8
 *
 * The chain is +0x28, then +0x24, then +0x18, and the object itself stays in r0
 * across the whole of it: the ROM never touches r0 between entry and the call.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a return value.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/call_with_chain_field.c
 */

#include "gba_types.h"

typedef struct ChainTail {
    u8  pad00[0x18];
    u32 value;                  /* +0x18 */
} ChainTail;

typedef struct ChainMid {
    u8         pad00[0x24];
    ChainTail *tail;            /* +0x24 */
} ChainMid;

typedef struct ChainHead {
    u8        pad00[0x28];
    ChainMid *mid;              /* +0x28 */
} ChainHead;

extern u32 FUN_0804c42c(ChainHead *head, u32 value);

/* 0x080500F8 */
u32 FUN_080500f8(ChainHead *head)
{
    return FUN_0804c42c(head, head->mid->tail->value);
}
