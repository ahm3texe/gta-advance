/* Pool +8 getters — 0x08037FAC-0x08037FFD
 *
 * Four identical leaf functions using different pool symbols, plus
 * FUN_08037FDC, which marks the active object with flag 0x8000 and clears
 * its sub-object if present.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/pool_gets.c
 */

#include "gba_types.h"

#define STATE_FLAG      0x8000

typedef struct Pool {
    u32 first;                  /* +0x00 */
    u8  pad04[4];
    u32 value;                  /* +0x08 */
} Pool;

typedef struct Obj {
    u8   pad00[12];
    u32  state;                 /* +0x0C */
    u8   pad10[12];
    void *sub;                  /* +0x1C */
} Obj;

extern Pool gUnk0202F310;
extern Pool gRam0202F300;
extern Pool gUnk02028280;
extern Pool gUnk0202F2C0;

extern void ReleaseActorEntries(void *sub);

/* 0x08037FAC */
u32 GetPoolA(void) { return gUnk0202F310.value; }

/* 0x08037FB8 */
u32 GetPoolB(void) { return gRam0202F300.value; }

/* 0x08037FC4 */
u32 GetPoolC(void) { return gUnk02028280.value; }

/* 0x08037FD0 */
u32 GetPoolD(void) { return gUnk0202F2C0.value; }

/* 0x08037FDC */
void MarkAndClear(Obj *obj)
{
    obj->state |= STATE_FLAG;
    if (obj->sub != 0)
        ReleaseActorEntries(obj->sub);
    obj->sub = 0;
}
