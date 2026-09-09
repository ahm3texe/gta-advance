/* The state value table — 0x08024044-0x0802418B (328 bytes)
 *
 * STATUS: 92/162 instructions, A NEAR MISS (does not match).  The size is
 * right.
 *
 * According to the object's +0x64 state it selects a u32 from the gRom08CA6138
 * table (gRom08CA617C when the sub-object is non-null) and returns it <<16.
 * 51 -> [9], 76 -> [8], 101 -> [15] if the +0x09 kind of the +0x84 sub-object
 * is 35 and [10] otherwise, 34 -> a 35-entry jump table on the +0x90 phase
 * (12..46): 12/38/39 -> [7], 22 -> the same selection as 101, 28 -> 0 if +0x8C
 * > 0x3FFFF and [4] otherwise, 36 -> 0, 46 -> [8]; everything else (18
 * included) indexes with the u16 of FUN_0804fb3c(sub)'s result.
 *
 * THE REMAINING DIFFERENCE, ONE MECHANISM: here agbcc MERGES identical bodies
 * (outer case 76 `[8]` with inner case 46 `[8]`, outer 101 with inner 22,
 * the inner default with the outer default) -- in the ROM these are SEPARATE
 * blocks (0x08024086/0x08024172, 0x0802408A/0x0802415C).  Yet the same ROM did
 * merge the two `return 0`s (0x08024150).  Tried: its own `return x << 16` in
 * every case (85), ordering the inner cases by the ROM's body layout (91),
 * linking the inner default to the outer one with a goto (79).  No source form
 * that prevents the merge was found; the inner switch is probably a separate
 * statement or helper.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/get_state_value.c
 */

#include "gba_types.h"
#define VAL_LIMIT   0x3FFFF
#define KIND_35     35
typedef struct Sub { u8 pad00[9]; u8 kind; } Sub;
typedef struct Obj { u8 pad00[0x64]; u8 state; u8 pad65[0x1F]; Sub *sub; u8 pad88[4]; s32 amount; s32 phase; } Obj;
extern const u32 gRom08CA6138[];
extern const u32 gRom08CA617C[];
extern u32 FUN_0804fb3c(Sub *sub);
u32 GetStateValue(Obj *obj, u32 alt)
{
    const u32 *tbl; Sub *sub; u32 v;
    sub = obj->sub;
    tbl = gRom08CA6138;
    if (alt != 0)
        tbl = gRom08CA617C;
    switch (obj->state) {
    case 51:
        v = tbl[9];
        break;
    case 76:
        v = tbl[8];
        break;
    case 101:
        if (sub != 0 && sub->kind == KIND_35)
            v = tbl[15];
        else
            v = tbl[10];
        break;
    case 34:
        switch (obj->phase) {
        case 12:
        case 38:
        case 39:
            v = tbl[7];
            break;
        case 22:
            if (sub != 0 && sub->kind == KIND_35)
                v = tbl[15];
            else
                v = tbl[10];
            break;
        case 28:
            if (obj->amount > VAL_LIMIT)
                return 0;
            v = tbl[4];
            break;
        case 36:
            return 0;
        case 46:
            v = tbl[8];
            break;
        default:
            v = tbl[(u16)FUN_0804fb3c(sub)];
            break;
        }
        break;
    case 18:
    default:
        v = tbl[(u16)FUN_0804fb3c(sub)];
        break;
    }
    return v << 16;
}
