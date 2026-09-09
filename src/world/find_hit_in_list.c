/* Searching the hit list — 0x08041F8C-0x08042009 (126 bytes)
 *
 * STATUS: 41/61 instructions, A NEAR MISS (does not match).  The size is
 * right.
 *
 * For each entry in the list the low two bits of the +0x06 flags are cleared
 * and FUN_08040564 is called with (x,y); if there is a result, 2 is added to
 * the flags when mask 10 is set in the +0x08 kind (plus 0x20 when bit1 of the
 * kind is set) and 1 otherwise, and the result is returned.  If nothing is
 * found the fallback is returned.
 *
 * THE REMAINING DIFFERENCE: the ROM also ORs r7 = 0 into the flags with
 * `orrs r1,r7` (`movs r7,#0` in the loop preheader, callee-saved).  So the
 * source has a zero-valued variable living across the loop.  Writing the
 * `extra = 0` local inside the loop (41), outside it (41/66) and hoisted
 * together with the mask (39) did not give the ROM's register allocation; the
 * `(count & 0)` trick gives 47 but is indefensible as source.  In the
 * allocation ctx is r8 and fallback r9 (r7/r8 here) -- there must be one more
 * variable.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/find_hit_in_list.c
 */

#include "gba_types.h"
typedef struct Hit { u16 x; u16 y; u8 pad04[2]; u16 flags; } Hit;
typedef struct Res { u8 pad00[8]; u8 kind; } Res;
extern Res *FUN_08040564(u32 ctx, u32 x, u32 y);
Res *FindHitInList(u32 ctx, Hit *list, s32 count, Res *fallback)
{
    s32 i; u32 extra; u32 m; u32 v; Res *r;
    for (i = 0; i < count; i++) {
        extra = 0;
        m = 0xFFFC;
        m = m & list->flags;
        list->flags = m;
        r = FUN_08040564(ctx, list->x, list->y);
        if (r != 0) {
            if (r->kind & 10) {
                v = list->flags | 2 | extra;
                list->flags = v;
                if (r->kind & 2) {
                    v |= 0x20;
                    list->flags = v;
                }
            } else {
                list->flags = 1 | list->flags;
            }
            return r;
        }
        list++;
    }
    return fallback;
}
