/* Havuz +8 getirici — 0x08037FAC-0x08037FFD
 *
 * Dort ozdes yaprak (her biri farkli havuz sembolu) ile aktif nesne
 * durumunu 0x8000 bayragiyla isaretleyip alt nesne varsa temizleyen
 * FUN_08037FDC.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/pool_gets.c
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
