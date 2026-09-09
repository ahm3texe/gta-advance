/* Release the selected node and mark the record's slot free - 0x08053E9C,
 * 206 bytes.
 *
 * There are two record pointers: 0x02026F34 (mode 2) and 0x020272C8 (the other
 * modes). The function ALWAYS first reads the +0x30 selected node of the
 * record at 0x020272C8 and its +0x24 attachment; then, depending on the mode,
 * it empties the corresponding record and adjusts its slot's +0x0C flags,
 * relinks the record to the slot (+0x2C) and performs a cleanup on the
 * selected node. The emptied slot is returned, or 0 on failure.
 *
 * Details MEASURED from the ROM
 * -----------------------------
 * - The single parameter arrives in r0 and is only tested with `cmp r0,#2`.
 * - `pop {r4,r5,r6}; pop {r1}; bx r1` -> r0 is live, so the return type is NOT
 *   void (the inverse of rule 35). The returned value is r5, the pointer to
 *   the emptied slot.
 * - Record layout: +0x0B flag byte (bit 1 is set), +0x28 slot, +0x30 selected
 *   node. Slot layout: +0x0C flag word (bits 0x40, 0x01, 0x80, 0x10), +0x2C
 *   back-pointer to the record.
 * - Node layout: +0x18 flag word (bit 0x400 built with
 *   `movs #0x80/lsls #3`), +0x24 attached record, +0x28 slot.
 * - `movs r2,#17; negs r2,r2` -> the mask is built at int width, so the slot
 *   flag is u32 (rule 47: a narrow field would fold to 0xEF).
 * - 0x08041EE0 takes no arguments (2 bytes in data/functions.csv,
 *   `void NoOp08041EE0(void)` in empty_stubs.c); the r0 before the call
 *   happens to hold the slot.
 * - 0x02026F34 and 0x020272C8 are declared `u32` in ram_map. As in the sibling
 *   nodelist_a7.c the type was NOT CHANGED; the address is taken and cast --
 *   so as not to create a contradictory extern type.
 *
 * WHY THE RELOADS HAPPEN (measured)
 * ---------------------------------
 * agbcc's CSE flushes the memory cache on EVERY store through a pointer. That
 * is why the ROM reads `(*base)` three times:
 *   - at the start (for the selected node),
 *   - after the `+0x30 = 0` store (for the +0x0B byte),
 *   - after the `+0x0B` store (for the tail).
 * In the source this corresponds to holding the pointer in a local in the
 * mode-2 branch (agbcc does not flush a local) and doing a SEPARATE read for
 * the tail; in the other branch the `(*secondary)` expression is used
 * directly. volatile is NOT NEEDED -- the `Ctx *volatile *` trick from a7 is
 * unnecessary here; it was tried and made no difference.
 *
 * FORMS TRIED AND ELIMINATED
 * --------------------------
 * 1. Declaring the `primary` address local at the TOP of the function: 206
 *    bytes but 41 differences. The address lives across the whole function and
 *    claims r7, making the prologue `push {r4,r5,r6,r7,lr}`; the ROM has no
 *    r7. Rule 16: it is not the declaration site but the ASSIGNMENT site that
 *    matters -- moving the assignment inside the mode-2 block removed r7
 *    (41 differences -> a structurally correct allocation).
 * 2. A single shared `u32 f` (one flag local shared by both branches): the
 *    allocation shifts. The ROM keeps the flag in r2 in the mode-2 branch and
 *    in r1 in the other, i.e. TWO SEPARATE locals (rule 45). Splitting them
 *    made the other branch match exactly.
 * 3. A single shared `ctx` plus a single `ctx->slot->owner = ctx;` line AFTER
 *    the if/else: 202 bytes, 4 SHORT. Because the two branches' reload
 *    instructions also fell on the same register pair (`ldr r1,[r3,#0]`),
 *    agbcc moved the cross-jump ONE INSTRUCTION EARLIER and merged the reload
 *    as well; in the ROM that instruction is physically present twice (in the
 *    mode-2 branch `ldr r1,[r1,#0]`, in the other `ldr r1,[r3,#0]` -- they
 *    cannot merge in the ROM because the address registers differ). The 2 lost
 *    bytes plus 2 bytes of pool alignment padding = 4.
 * 4. Making the tail read `*(Ctx *volatile *)&gRam02026F34` (rule 39): 202
 *    again. The volatile flag does NOT PREVENT cross-jumping.
 * 5. RULE 45 SOLVED IT: each branch was given its own `ctx` local and the
 *    `ctx->slot->owner = ctx;` line was written in BOTH branches. The shared
 *    final two instructions (`ldr r0,[r1,#0x28]; str r1,[r0,#0x2c]`) still
 *    merge by cross-jumping -- the ROM's `b 0x8053F20` is exactly that merge
 *    -- but the reload stayed in each branch. 0 differences.
 *
 * MATCH: 206/206 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_a8.c
 */

#include "gba_types.h"

struct Ctx;

/* The slot; the +0x0C flag word and the +0x2C back-pointer are used here. */
typedef struct Slot {
    u8          pad00[0x0C];
    u32         flags;          /* +0x0C */
    u8          pad10[0x1C];
    struct Ctx *owner;          /* +0x2C */
} Slot;

/* The record the attachment points at; only its +0x1C field is checked for zero. */
typedef struct Detail {
    u8    pad00[0x1C];
    void *unk1C;                /* +0x1C */
} Detail;

/* The +0x24 attachment of the selected node. */
typedef struct Extra {
    u8      pad00[0x14];
    Detail *detail;             /* +0x14 */
} Extra;

typedef struct ListNode {
    u8     pad00[0x18];
    u32    flags;               /* +0x18, bit 0x400 is tested */
    u8     pad1C[8];
    Extra *extra;               /* +0x24 */
    Slot  *slot;                /* +0x28 */
} ListNode;

typedef struct Ctx {
    u8        pad00[0x0B];
    u8        dirty;            /* +0x0B, bit 1 is set */
    u8        pad0C[0x1C];
    Slot     *slot;             /* +0x28 */
    u8        pad2C[4];
    ListNode *sel;              /* +0x30 */
} Ctx;

/* A SYMBOL DECLARATION THAT IS NEEDED (not written, reported instead): both
 * stand as `u32` in ram_map, yet they hold Ctx pointers. Changing the type
 * would create a contradictory extern with src/world/node_search.c and
 * src/core/nodelist_a7.c, so the existing type was kept and the address
 * taken. */
extern u32 gRam02026F34;            /* 0x02026F34 */
extern u32 gRam020272C8;            /* 0x020272C8 */

extern s32  FUN_08055888(ListNode *node, s32 mode);
extern void NoOp08041EE0(void);

/* 0x08053E9C */
Slot *ReleaseSelectedNode(s32 kind)
{
    Ctx **secondary = (Ctx **)&gRam020272C8;
    Ctx *active;
    Ctx *ctxPrimary;
    Ctx *ctxSecondary;
    ListNode *sel;
    Extra *extra;
    Slot *slot;
    Slot *held;

    sel = (*secondary)->sel;
    extra = 0;
    if (sel != 0)
        extra = sel->extra;

    if (kind == 2) {
        /* Rule 16: the address assignment goes INSIDE THE BLOCK; at the top
           of the function its lifetime grows, it claims r7 and the prologue
           gets larger. */
        Ctx **primary = (Ctx **)&gRam02026F34;
        u32 primaryFlags;

        active = *primary;
        if (active->sel == 0)
            return 0;
        slot = active->slot;
        primaryFlags = slot->flags;
        if (primaryFlags & 0x40)
            return 0;
        if (primaryFlags & 1)
            return 0;
        active->sel = 0;
        slot->flags = primaryFlags | 0x80;
        active->dirty |= 1;
        /* The store above flushes the cache: the ROM RE-READS the record
           here. Rule 45: this local is branch-specific. */
        ctxPrimary = *primary;
        ctxPrimary->slot->owner = ctxPrimary;
    } else {
        u32 secondaryFlags;

        if (*secondary == 0)
            return 0;
        if (sel == 0)
            return 0;
        slot = (*secondary)->slot;
        secondaryFlags = slot->flags;
        if (secondaryFlags & 0x40)
            return 0;
        (*secondary)->sel = 0;
        if (!(secondaryFlags & 1))
            slot->flags = secondaryFlags | 0x80;
        (*secondary)->dirty |= 1;
        ctxSecondary = *secondary;
        ctxSecondary->slot->owner = ctxSecondary;
    }

    if (sel->slot != 0) {
        if ((sel->flags & 0x400)
         || (extra != 0 && extra->detail != 0 && extra->detail->unk1C != 0)) {
            if (FUN_08055888(sel, 0) == 0) {
                held = sel->slot;
                held->flags &= ~0x10;
                NoOp08041EE0();
            }
        }
    }
    return slot;
}
