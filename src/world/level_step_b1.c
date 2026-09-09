/* Releasing the pending nodes in a two-level entity list
 * 0x080151C0-0x0801523B, 124 bytes  [MATCHED]
 *
 * What the ROM does:
 *   Starting from the head pointer at 0x020230A0 it walks the outer list
 *   linked through +0x3C.  At each outer node it filters the +0x30 field by
 *   "is this a valid ROM pointer".  A node that passes the filter is also the
 *   HEAD of the inner list: the inner list is linked through +0x44 and its
 *   first element is the outer node itself.  For every node in the inner list
 *   whose +0x27 byte is 1:
 *     UnlinkToFree(node->unk18); node->unk18 = 0;
 *     kind &= node->flags20;   (kind is 1 here, i.e. a bit 0 test)
 *     if kind and node->unk34 are set, FUN_08013308(node->unk34);
 *     FUN_0801362c(node->unk38);
 *     node->kind27 = 0;
 *
 * DETAILS MEASURED FROM THE ROM
 * -----------------------------
 * (1) THE ZERO CHECK MUST BE AN EXPLICIT BRANCH -- THE ONE MEASUREMENT THAT
 *     OPENED THE MATCH.  The ROM does `cmp r1,#0 / beq` first and filters the
 *     range afterwards.  The arithmetic test would already reject zero
 *     (0 - 0x08000000 = 0xF8000000 > 0xFFFFFF), and written as
 *     `if (addr != 0 && addr - ROM_BASE <= ROM_SPAN)` agbcc DELETES the first
 *     branch ENTIRELY: 120 bytes, four short.  Splitting the two conditions
 *     into separate `if (...) goto next;` statements brought the branch back.
 *     A side effect: the same change also moved the register the zero constant
 *     is copied into from r1 to the ROM's r0 (those two instructions had been
 *     different on their own as well).
 * (2) 0xF8000000 is built with `movs r0,#248 / lsls r0,#24` rather than loaded
 *     from the pool; that is why the condition is written as
 *     `addr - ROM_BASE`.  The upper bound (0x00FFFFFF), by contrast, is read
 *     from the pool.
 * (3) Both loops use an ENTRY GUARD + a form that loops from the bottom.  As
 *     rule 49 warns, the loop form of the sibling SelectEntityHandler
 *     (0x0801528C) was NOT COPIED; the form was read from the ROM
 *     disassembly.  There is no rare body at the end of the function; the
 *     `goto next` only goes to the outer loop's step point.
 * (4) `movs r0,#0 / mov r8,r0` sits in the inner loop's PREHEADER: agbcc
 *     hoists the two zero constants (the unk18 and kind27 writes) out as loop
 *     invariants.  Because r8 is used, the prologue has
 *     `mov r7,r8 / push {r7}`.  There is NO separate `zero` local in the
 *     source; two plain 0 constants suffice.
 * (5) `ldrb r4 / cmp #1 / ... / ands r4,r0` -- the AND result stays in the
 *     same register, i.e. a compound assignment in rule 33's form
 *     (`kind &= ...`).  The `if (kind & node->flags20)` spelling was not
 *     tried; the compound assignment already gave the ROM.
 * (6) Because the +0x27 offset exceeds the ldrb/strb immediate limit (#31),
 *     agbcc takes the address into a separate register (r7) by itself; there
 *     is NO need to write a pointer local in the source.
 *
 * A SPELLING TRIED AND REJECTED
 * -----------------------------
 * - `if (addr != 0 && addr - ROM_BASE <= ROM_SPAN) { ... }`: 120 bytes, the
 *   two conditions merge and the zero branch disappears (see (1) above).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/level_step_b1.c
 */

#include "gba_types.h"
#include "sprite_pool.h"

void UnlinkToFree(Node *node);
void FUN_08013308(void *arg);
void FUN_0801362c(void *arg);

typedef struct Entry {
    u8            pad00[0x18];
    Node         *unk18;        /* +0x18 the node handed to UnlinkToFree */
    u8            pad1C[4];
    u16           flags20;      /* +0x20 */
    u8            pad22[5];
    u8            kind27;       /* +0x27 */
    u8            pad28[8];
    u32           unk30;        /* +0x30 filtered as a ROM pointer */
    void         *unk34;        /* +0x34 */
    void         *unk38;        /* +0x38 */
    struct Entry *next3c;       /* +0x3C the outer list link */
    u8            pad40[4];
    struct Entry *next44;       /* +0x44 the inner list link */
} Entry;

/* The range in which the +0x30 field counts as valid: 0x08000000-0x08FFFFFF. */
#define ROM_BASE 0x08000000
#define ROM_SPAN 0x00FFFFFF

/* 0x020230A0: the outer list's head pointer.  Since I am not authorised to
   add a symbol
   Since I am not authorised to ADD one, a constant cast was written; because
   the offset is 0,
   rule 1's folding problem does not arise here (the ROM also reads the
   address from the pool in one piece and does `ldr r6,[r0,#0]`). */
#define gListHead020230A0 (*(Entry **)0x020230A0)

/* 0x080151C0 */
void ReleaseEntryResources(void)
{
    Entry *outer;
    Entry *node;
    u32 addr;
    u32 kind;

    outer = gListHead020230A0;
    while (outer != 0) {
        addr = outer->unk30;
        if (addr == 0)
            goto next;
        if (addr - ROM_BASE > ROM_SPAN)
            goto next;

        node = outer;
        while (node != 0) {
            kind = node->kind27;
            if (kind == 1) {
                UnlinkToFree(node->unk18);
                node->unk18 = 0;
                kind &= node->flags20;
                if (kind != 0 && node->unk34 != 0)
                    FUN_08013308(node->unk34);
                FUN_0801362c(node->unk38);
                node->kind27 = 0;
            }
            node = node->next44;
        }
    next:
        outer = outer->next3c;
    }
}
