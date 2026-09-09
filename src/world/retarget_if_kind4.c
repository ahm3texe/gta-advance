/* Retargeting the handler when the kind is 4 — 0x0803F6D8-0x0803F6F7
 *
 * Reads and saves the first field BEFORE THE CALL, calls FUN_0803CDA8, then
 * tests the +8 bytes of the saved object and replaces the handler at +0x08 if
 * it is 4.  Because the object pointer must LIVE across the call, it is held
 * in a separate local (r4 in the ROM).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/retarget_if_kind4.c
 */

#include "gba_types.h"


#define KIND_DONE 4

typedef struct Obj {
    u8 pad00[8];
    u8 kind;                    /* +0x08 */
} Obj;

typedef struct Holder {
    Obj  *first;                /* +0x00 */
    u8    pad04[4];
    void *handler;              /* +0x08 */
} Holder;

extern void FUN_0803cda8(Holder *holder);
/* The THUMB BIT (bit 0) must be set in a stored function pointer.
   a symbol with the `__thumb` suffix; its address resolves as | 1
   (tools/agbcc_build.py).  Adding the bit at a `bl` target would corrupt the
   branch offset,
   would break, a separate symbol is used. */
extern u8 FUN_0803ef18__thumb[];

/* 0x0803F6D8 */
void RetargetIfKind4(Holder *holder)
{
    Obj *obj;

    obj = holder->first;
    FUN_0803cda8(holder);
    if (obj->kind == KIND_DONE)
        holder->handler = FUN_0803ef18__thumb;
}
