/* Set up the focus block with a new target -- 0x0800A8A8-0x0800A92F  (136 bytes, MATCHED)
 *
 * If gSlotSelector is 1, slots 1 and 2 are SWAPPED; the target is then written
 * either into the "held" slot (slot == 2, handing the previously held one over
 * to FUN_0803220c) or into CoordBlock +0x04. If slot <= 1 the block is
 * initialised: +0x00 = 1, +0x3C/+0x40/+0x44 = 0, +0x0C = code << 16, +0x71 =
 * the third parameter.
 *
 * The code comes from one of two sources depending on the target's +0x08 flag
 * byte:
 *   bit 0x30 set  -> the low TWO bits of (*(target+0x20))[+0x1F], << 8
 *   otherwise     -> the halfword (*(target+0x18))[+0x0E] & 0x3FF
 *
 * Rule 35: the final `pop {r0}; bx r0` says the return type is void.
 * Rule 31: `cmp r5,#1 / bhi` is an UNSIGNED branch, so the slot parameter is
 * u32.
 *
 * ================= THE FOUR MEASUREMENTS THAT CLOSED THE GAP =============
 *
 * (1) THE THIRD PARAMETER MUST BE `u32`, NOT `u8` (rule 15).  144 -> 136 bytes.
 *     A `u8 value` inserted an `lsls r2,#24 / lsrs r7,r2,#24` normalization at
 *     entry; the ROM has only `adds r7,r2,#0`.  Because the field (+0x71) is
 *     still u8, the store is still `strb`.
 *
 * (2) THE LOW TWO BITS MUST BE WRITTEN AS A BITFIELD (rule 24).  140 -> 136
 *     bytes, 54 -> 19 differences.
 *     ROM: `ldrb r0,[r0,#31] / lsls r0,#30 / lsrs r2,r0,#22` -- that is,
 *     (field & 3) << 8 fused into a SINGLE shift pair.  With an explicit mask
 *     (`(x & 3) << 8`) agbcc produces four instructions:
 *     `movs r0,#3 / ldrb r1 / ands r0,r1 / lsls r1,r0,#8`.  Declaring
 *     `u8 unk1F : 2;` produces a zero_extract and enables the fusion.
 *
 * (3) THE BLOCK BASE MUST NOT BE TAKEN INTO A LOCAL POINTER (an application of
 *     rule 50).  19 -> 7 differences.
 *     Writing `CoordBlock *block = &gRam02011030;` pushed the base down to the
 *     GLOBAL allocator; dump_alloc: refs 7 / lifetime 54 -> priority 0.259,
 *     leaving it 4th in order and taking r2.  `code` (refs 5 / lifetime 8 ->
 *     1.250) was allocated first and claimed r1 -- the INVERSE of the ROM.
 *     Accessing the member DIRECTLY as `gRam02011030.field` confines the base
 *     address to a single basic block, drops it to the LOCAL allocator (L11)
 *     and takes r1 BEFORE the allocation race; `code` takes the remaining r2.
 *     Both settle onto the ROM by themselves.
 *
 * (4) THE `unk18` READ MUST BE A SEPARATE STATEMENT (rule 18/25).  4 -> 0
 *     differences.
 *     The ROM's order: `ldr r0,[r6,#24] / ldr r2,=0x3ff / ldrh r0,[r0,#14] /
 *     ands r2,r0` -- the pointer FIRST, the constant SECOND.  `code = 0x3FF;
 *     code &= actor->unk18->unk0E;` moved the constant ahead.  The intermediate
 *     statement `info = actor->unk18;` gives the ROM's order.
 *
 * ============== FORMS TRIED AND ELIMINATED (do not repeat) ==============
 *
 * - A SINGLE SHARED `block` LOCAL (the same variable for block 2 and block 3,
 *   with two separate assignments).  It looked promising because the ROM emits
 *   two separate `ldr r1,=0x02011030`; the measurement said otherwise: as one
 *   pseudo, refs 10 / lifetime 128 -> priority 0.234, which falls back even
 *   further.  19 differences, unchanged.
 * - `mask = 0x30; mask &= actor->kind; if (mask != 0)` (the rule 33 form):
 *   correct but UNNECESSARY.  `if ((actor->kind & 0x30) != 0)` produces THE
 *   SAME `movs r0,#48 / ldrb r2 / ands r0,r2 / cmp r0,#0` sequence; the shorter
 *   one was chosen.
 * - A local base pointer for blocks 2 and 6: in block 6 (the call branch) both
 *   a local and direct access MATCH -- the base there already crosses the call
 *   and goes to callee-saved r4, so the source form makes no difference.
 *   In block 2, however, a local pointer gave r0 while the ROM wants r1: direct
 *   access is required.
 *
 * NOTE -- the line `if (slot == 0) gRam02011030.unk08 = 0;` comes out as
 * `str r5,[r1,#8]` in the ROM, i.e. the slot register is used instead of the
 * constant 0.  We did not write that: CSE records the slot == 0 equality from
 * the branch condition and substitutes that register for the zero.
 *
 * The CoordBlock definition must be BYTE-FOR-BYTE the same as in
 * src/core/read_triple.c, src/core/update_focus.c,
 * src/core/clear_coord_byte71.c, src/misc/coord_accessors.c and
 * src/misc/coord_more.c (TYPES-001).  That is why the +0x3C/+0x40/+0x44 words
 * were not carved out of the body but are written through a cast inside pad3C;
 * changing the body would break those five files.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/clip_bounds_b1.c
 */

#include "gba_types.h"

typedef struct CoordBlock {
    u32 unk00;                  /* +0  */
    u32 unk04;                  /* +4  */
    u32 unk08;                  /* +8  */
    s32 second;                 /* +12 */
    s32 first;                  /* +16 */
    u8  pad14[28];
    u32 a;                      /* +0x30 */
    u32 b;                      /* +0x34 */
    u32 c;                      /* +0x38 */
    u8  pad3C[12];
    u32 unk48;                  /* +72 */
    u8  pad4C[37];
    u8  byte71;                 /* +0x71 */
} CoordBlock;

/* Of the blocks pointed at by the target's +0x18 and +0x20 pointers, ONLY the
   fields read here were expanded; the rest is padding of unknown meaning. */
typedef struct InfoA {
    u8  pad00[14];
    u16 unk0E;                  /* +0x0E */
} InfoA;

typedef struct InfoB {
    u8  pad00[31];
    u8  unk1F : 2;              /* +0x1F, low two bits (measured value: 2) */
} InfoB;

typedef struct Actor {
    u8     pad00[8];
    u8     kind;                /* +0x08 */
    u8     pad09[15];
    InfoA *unk18;               /* +0x18 */
    u8     pad1C[4];
    InfoB *unk20;               /* +0x20 */
} Actor;

extern CoordBlock gRam02011030;
extern u16        gSlotSelector;

extern void FUN_0803220c(Actor *actor, Actor *previous);

/* 0x0800A8A8 */
void BindActorToCoordSlot(Actor *actor, u32 slot, u32 value)
{
    u32 sel;

    sel = gSlotSelector;
    if (sel == 1) {
        if (slot == 2)
            slot = 1;
        else if (slot == 1)
            slot = 2;
    }

    if (slot == 2) {
        FUN_0803220c(actor, (Actor *)gRam02011030.unk08);
        gRam02011030.unk08 = (u32)actor;
    } else {
        if (slot == 0)
            gRam02011030.unk08 = 0;
        gRam02011030.unk04 = (u32)actor;
    }

    if (slot <= 1) {
        InfoA *info;
        u32 code;

        gRam02011030.unk00 = 1;
        *(u32 *)&gRam02011030.pad3C[0] = 0;
        *(u32 *)&gRam02011030.pad3C[4] = 0;
        *(u32 *)&gRam02011030.pad3C[8] = 0;

        if ((actor->kind & 0x30) != 0) {
            code = actor->unk20->unk1F << 8;
        } else {
            info = actor->unk18;
            code = 0x3FF;
            code &= info->unk0E;
        }

        gRam02011030.second = code << 16;
        gRam02011030.byte71 = value;
    }
}
