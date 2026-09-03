/* Nesne durum sorgulari — 0x08019670-0x0801970F
 *
 * Dort yardimci. Ilki bir olcegi moda gore 16/32/51 ile carpip 6 bit
 * kaydiriyor (51 = 3 * 17, agbcc bunu `(v<<1)+v` ve `x + (x<<4)` olarak
 * uretiyor). Digerleri +28'deki alt nesnenin +10 ve +167 baytlarina bakiyor.
 *
 * Son iki fonksiyon govde olarak ayni, yalnizca cagirdiklari sorgu farkli.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/object_state.c
 */

#include "gba_types.h"

#define MODE_NARROW      1
#define MODE_WIDE        2
#define SCALE_NARROW     32
#define SCALE_WIDE       16
#define SCALE_DEFAULT    51
#define SCALE_SHIFT      6

#define SUB_STATE_LOCKED 3
#define SLOT_UNSET       0xFF
#define SUB_SLOT_OFFSET  167
#define QUERY_KIND       48

typedef struct ObjSub {
    u8 pad00[10];
    u8 state;               /* +10 */
    u8 pad0B[SUB_SLOT_OFFSET - 11];
    u8 slot;                /* +167 */
} ObjSub;

typedef struct Obj {
    u8      pad00[28];
    ObjSub *sub;            /* +28 */
} Obj;

extern s32  FUN_08038084(Obj *o);
extern u32  FUN_08025fc0(Obj *o, int kind);
extern void FUN_08028ca8(u8 slot);
extern u32  FUN_0805af48(Obj *o);
extern u32  FUN_0805aefc(Obj *o);

/* 0x08019670 */
s32 ScaleByMode(Obj *o, int mode)
{
    s32 value;
    s32 scaled;

    value = FUN_08038084(o);

    if (mode == MODE_WIDE)
        scaled = value * SCALE_WIDE;
    else if (mode != MODE_NARROW)
        scaled = value * SCALE_DEFAULT;
    else
        scaled = value * SCALE_NARROW;

    return scaled >> SCALE_SHIFT;
}

/* 0x0801969C */
void ReleaseObjectSlot(Obj *o)
{
    if (o == 0)
        return;
    if (FUN_08025fc0(o, QUERY_KIND) == 0)
        return;

    FUN_08028ca8(o->sub->slot);
    o->sub->slot = SLOT_UNSET;
}

/* 0x080196C8 */
u32 IsObjectReadyA(Obj *o)
{
    ObjSub *sub;

    if (FUN_0805af48(o) == 0)
        return 0;

    sub = o->sub;
    if (sub != 0 && sub->state == SUB_STATE_LOCKED)
        return 0;

    return 1;
}

/* 0x080196EC */
u32 IsObjectReadyB(Obj *o)
{
    ObjSub *sub;

    if (FUN_0805aefc(o) == 0)
        return 0;

    sub = o->sub;
    if (sub != 0 && sub->state == SUB_STATE_LOCKED)
        return 0;

    return 1;
}
