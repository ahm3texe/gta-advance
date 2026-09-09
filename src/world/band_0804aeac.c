/* Target selection + the near-danger notification — FUN_0804aeac @ 0x0804AEAC,
 * 476 bytes
 *
 * The same family as its sibling AdvanceTargetAction (0x0804B088, matched): it
 * uses the `linked` / `nearest` / `distance` inline helpers from
 * `include/target_common.h` as they are, and the structs and style come from
 * there.
 *
 * The structure:
 *   1. If context->details->target is set, the target is that candidate's
 *      link/owner/context chain (`linked`); if it is empty, the nearest of two
 *      global candidates by octagonal approximate distance (`nearest`).
 *   2. If the mount (self->link) does not carry the 0x100000 flag AND the
 *      progress counter (details +0x0A) is zero and GetOwnerSlot validates the
 *      target, FUN_080502f8 is called with code 31 (the lockup/warning
 *      notification).
 *   3. In every case it returns the result of FUN_0804b2bc(self).
 *
 * TWO MEASURED SOURCE LEVERS (both required for byte-matching):
 *
 * (a) The `actor = self` copy.  On entry the ROM keeps TWO copies of the
 *     parameter with `adds r6,r0,#0 / adds r5,r6,#0`: r6 for the
 *     context/link/final call, r5 for the distance measurement in the
 *     `nearest` body and the gRam0202F3D8 write.  Written with a single
 *     variable the two pseudos merge into one register, the copy instruction
 *     disappears and the drift spreads through the whole function: 152/227.
 *     With a separate `actor` local, 213/227.  (The sibling
 *     AdvanceTargetAction does NOT have this copy -- there `self` is a single
 *     pseudo; rule 37's "a copy does not survive" limit depends on register
 *     pressure, it is not absolute.)
 *
 * (b) The REUSE of the `node` local.  The ROM really does build the counter
 *     pointer with `adds r2,r0,#0 / adds r2,#10`.  Written plainly as
 *     `phase=&details->phase`, agbcc's cse pass folds the address into the
 *     point of use (`ldrh r0,[r2,#10]`) and two bytes are lost.  The root
 *     cause was measured: with `-fno-cse-skip-blocks` the difference
 *     disappears, i.e. cse SKIPS the mount check's block and still sees the
 *     base pseudo as live.  The only lever in the source is to REASSIGN the
 *     base pseudo IN BETWEEN: `node` holds details first and the mount
 *     afterwards; the second assignment invalidates cse's equivalence and the
 *     ROM's two-instruction address setup comes back.
 *
 * SPELLINGS RULED OUT (all measured, none of them opened (b) -- it stayed at
 * 213/227):
 *   computing phase early/late; a separate local named `details`;
 *   `(u16 *)((u8 *)d+10)` instead of `&d->phase`; the mount check written
 *   nested / with `||` / with a flag local; moving the tail into a separate
 *   `static __inline__` function; making phase a `volatile u16 *`; two
 *   separate assignments; an array/sub-struct view; reading `*phase` into a
 *   local; changes to the declaration order; adding a second use with
 *   `*phase=1` (cse folds it anyway, so the problem is not the NUMBER of
 *   uses).
 *   `s16 *phase` (215/227) broke the fold but produced `ldrsh r0,[r2,r1]` --
 *   the wrong instruction; it did show, though, that the rest of the block is
 *   identical to the ROM.
 *   `phase=(u16 *)context->details; phase+=5;` (217/227), a magic offset,
 *   ruled out.
 *   Reading the mount late with `node=(TargetNode *)self->link`: 168/227 (the
 *   load moves away from the top of the function, whereas the ROM reads it
 *   from the cache with `mov r0,sl`).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification: make c-match FILE=src/world/band_0804aeac.c  -> BYTE-MATCHING
 */

#include "target_common.h"

#define MOUNT_BUSY  (0x80 << 13)    /* 0x00100000 */
#define NOTIFY_KIND 31

/* A shared view for details (+0x24) and the mount (+0x2C): both carry a half
 * word at +0x0A and a full word at +0x18. */
typedef struct TargetNode {
    u8  pad0[10];
    u16 phase;                  /* +0x0A */
    u8  pad12[12];
    u32 flags;                  /* +0x18 */
} TargetNode;

extern u32  GetOwnerSlot(TargetActor *actor);
extern void FUN_080502f8(u32 kind, TargetActor *actor);
extern u32  FUN_0804b2bc(TargetActor *self);

u32 FUN_0804aeac(TargetActor *self)
{
 TargetActor *actor=self;
 TargetContext *context=self->context;
 TargetActor *mount=self->link;
 TargetActor *candidate=context->details->target;
 TargetActor *target;
 TargetNode *node;
 u16 *phase;
 if(candidate) target=linked(candidate); else target=nearest(actor);
 node=(TargetNode *)context->details;
 phase=&node->phase;
 node=(TargetNode *)mount;
 if(node && (node->flags&MOUNT_BUSY)) goto done;
 if(*phase) goto done;
 if(!GetOwnerSlot(target)) goto done;
 FUN_080502f8(NOTIFY_KIND,target);
done:
 return FUN_0804b2bc(self);
}
