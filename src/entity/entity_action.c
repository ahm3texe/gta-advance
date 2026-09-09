/* Entity action attempt — 0x08020A9C-0x08020B6D
 *
 * The name is not known yet. FUN_080208a8 is attempted twice on the entity's
 * sub-object at 0x64; depending on the result and the flags, either 2 or 8 is
 * written to the state byte at 0x114.
 *
 * The ROM's tail has three paths that all reach the same store: the flag test
 * and the state test change nothing, only the address computation is repeated
 * on one path. The form was kept as it is in the ROM.
 *
 * DOES NOT MATCH YET: 85 of 98 instructions are identical. Everything up to
 * the prologue and the first call (0x08020A9C-0x08020ACE) matches exactly.
 * Two differences remain:
 *   - Where the `return 0` block sits: the ROM places it early (0x8020AD2)
 *     and branches to it from four places; we place it at the end and branch
 *     forwards.
 *   - The direction of the tail branch: the ROM goes to the store with `bls`,
 *     we go to the else with `bhi`.
 *
 * Tried: `==0 return` / `!` / wrapping the body in an if (151/151/140 bytes;
 * the last was chosen), a three-way tail (150-151), an inverted condition
 * (140), a single if (140), and an unconditional store (133 bytes but only
 * 76/98 instructions -- the byte count is misleading because of branch
 * offsets, the alignment metric is more reliable).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entity/entity_action.c
 */

#include "gba_types.h"

#define FLAG_SKIP_RETRY   0x00440000
#define FLAG_ALLOW_BLOCK  0x00100000
#define FLAG_FORCE_STATE  0x00040000

#define STATE_BLOCKED     2
#define STATE_DONE        8
#define STATE_BLOCK_MAX   1
#define STATE_DONE_MAX    3

#define SUB_OFFSET        0x13C

typedef struct Entity {
    u8   pad00[0x64];
    u32  sub;               /* +0x64 */
    u8   pad68[0xAC];
    u8   state;             /* +0x114 */
} Entity;

extern u32 GetOwnerSlot(u32 sub);
extern u32 FUN_08023660(u32 sub);
extern u32 FUN_080208a8(Entity *e, void *b, u32 flags, int *a,
                        int *c, int *d, int *f, void *owner);

u32 FUN_08020a9c(Entity *e, void *b, int unused, u32 flags)
{
    int slot0;
    int slot1;
    int slot2;
    int slot3;

    GetOwnerSlot(e->sub);
    if (FUN_080208a8(e, b, flags, &slot0, &slot1, &slot2, &slot3, e) != 0) {

        if ((flags & FLAG_SKIP_RETRY) == 0) {
            GetOwnerSlot(e->sub);
            if (FUN_080208a8(e, b, flags, &slot0, &slot1, &slot2, &slot3,
                             (u8 *)e + SUB_OFFSET) != 0) {
                GetOwnerSlot(e->sub);
                return 0;
            }
        }

        if (GetOwnerSlot(e->sub) != 0 && FUN_08023660(e->sub) != 0
            && (flags & FLAG_ALLOW_BLOCK) != 0) {
            if (e->state > STATE_BLOCK_MAX)
                return 0;
            e->state = STATE_BLOCKED;
            return 0;
        }

        if ((flags & FLAG_FORCE_STATE) != 0 || e->state <= STATE_DONE_MAX)
            e->state = STATE_DONE;
        else
            e->state = STATE_DONE;

        return 1;
    }

    return 0;
}
