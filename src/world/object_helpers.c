/* Object helpers — 0x08038358-0x080383B7
 *
 * Four small helpers: delegate to a sub-object, query a flag, run an update
 * chain, and read an ID with a default.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/object_helpers.c
 */

#include "gba_types.h"

#define OBJ_FLAG_ACTIVE   0x0001
#define OBJ_FLAG_BLOCKED  0x0840
#define OBJ_KIND_UPDATE   2
#define ID_NONE           0x7FEF

typedef struct ObjLink {
    u8  pad00[8];
    u16 id;                 /* +8 */
} ObjLink;

typedef struct Object {
    u8       pad00[8];
    u8       kind;          /* +8 */
    u8       pad09[3];
    u32      flags;         /* +12 */
    u8       pad10[8];
    u32      sub;           /* +24 */
    u8       pad1C[16];
    ObjLink *link;          /* +44 */
} Object;

extern u32  FUN_08036658(u32 sub);
extern void FUN_080367cc(Object *o);
extern void FUN_080366e0(Object *o);
extern void FUN_08038888(Object *o);

/* 0x08038358 */
u32 ForwardToSub(Object *o)
{
    return FUN_08036658(o->sub);
}

/* 0x08038364 */
u32 IsObjectActive(Object *o)
{
    u32 flags;

    flags = o->flags;
    if ((flags & OBJ_FLAG_ACTIVE) == 0)
        return 0;
    if ((flags & OBJ_FLAG_BLOCKED) != 0)
        return 0;

    return 1;
}

/* 0x08038380 */
void UpdateObject(Object *o)
{
    FUN_080367cc(o);
    FUN_080366e0(o);
    if (o->kind == OBJ_KIND_UPDATE)
        FUN_08038888(o);
}

/* 0x080383A0 */
u16 GetObjectLinkId(Object *o)
{
    if (o == 0)
        return ID_NONE;
    if (o->link == 0)
        return ID_NONE;

    return o->link->id;
}
