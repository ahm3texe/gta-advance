/* FUN_0803afbc — 0x0803AFBC-0x0803B19B (480 bytes)
 *
 * MATCHES (byte-matching, 480/480 bytes, 231/231 instructions identical).
 *
 * WHAT IT DOES: a "scene transition" step for the context pointed at by
 * gRam02000F08.  There are five gates first (the +0xAE flag bit 1, the busy
 * flag +0x08 and three ActorIdMatches queries 0x400C / 0x4009 / 0x400B); if any
 * of them holds, it returns without doing anything.  It then selects a wait
 * threshold according to the node's (+0x1C) state (state 8/9 -> 18, 10 -> 24)
 * and returns again if the step counter at +0xB8 is greater than zero but below
 * the threshold.  Once the gates are passed: if the node's +0x02 flag is still
 * zero it consults GetRamType() and sets the flag; if it could not be set, it
 * sends notification 348 or 349 with FUN_08035058 according to the state and
 * returns.  With the flag set there are two paths: if the state is 13 it copies
 * the position of the object at +0xB4 (either +0x18 or +4 of +0x20, depending on
 * flag 0x30) into the context's +0x10 as 12 bytes and calls FUN_08035f1c with
 * the active slot; otherwise it sets the phase to 2, clears +0xBC, passes the
 * node's index in the array ((node - slots) >> 3) and the absolute value of the
 * player's speed to FUN_0803a554, calls FUN_08030a3c if
 * gRam02000F00 == gSlotSelector + 1, and finally sets the +0x06 timer to 900 and
 * the +0xB8 step to 1.
 *
 * ---------------------------------------------------------------------
 * DETAILS READ FROM THE ROM
 *
 *  1. `node` (+0x1C) IS A LOCAL VARIABLE; everything else is read through the
 *     global.  It is taken at the very start at 0x803AFC2 with
 *     `ldr r5,[r2,#28]`, and r5 lives across three `bl`s up to 0x803B176.
 *     Because the calls clobber memory, that is only possible if it is a REAL
 *     local; by contrast the three places that read the state byte (0x803B00E,
 *     0x803B06A, 0x803B0C8) reload the whole `gRam02000F08->node->state` chain
 *     each time.  The two forms are kept apart deliberately: writing
 *     `node->state` would use r5 and the three `ldr` pairs would disappear.
 *
 *  2. THE THRESHOLD SELECTION IS A SWITCH, NOT AN if/else.  At 0x803B014 there
 *     are `cmp #8 / blt`, `cmp #9 / ble` and `cmp #10 / bne`: agbcc's sparse
 *     switch decision tree (rule 46).  Writing `if (s == 8 || s == 9)` would
 *     fold into a range test (`subs #8 / cmp #1 / bhi`) under rule 60.
 *     `default:` is a direct `goto ready;` -- the ROM's `blt L_b050`.
 *     THE CASE ORDER matters: in the ROM the `movs r2,#24` body sits right
 *     after the tree (as a fall-through) and the `movs r2,#18` body right
 *     before the shared code; so the source has `case 10` first, then
 *     `case 8/9`.
 *
 *  3. +0xB8 IS READ TWICE (rule 55).  `ldrb r0,[r1,#0] / cmp #0 / beq` and
 *     immediately afterwards `ldrb r1,[r1,#0] / cmp r1,r2 / bge` again.
 *     Copying it into a local deletes that second `ldrb`.  The address
 *     (p + 0xB8), on the other hand, is shared by CSE -- both come out of a
 *     single `gRam02000F08->step` writing.
 *
 *  4. THE MAIN SWITCH DOES PRODUCE A JUMP TABLE: state - 3, `cmp #8 / bls`,
 *     `lsls #2` and the 9-word table at 0x0803B08C.  The table is inside this
 *     function's own literal pool, not a data symbol.
 *     Cases 8 and 9 are ABSENT from the source; their table entries point at
 *     the default label (0x803B18A) -- because the table fills the min..max
 *     range.
 *     The body of case 3/5 DOES NOT STAND SEPARATELY in the ROM: agbcc's
 *     cross-jumping merged it with the identical block at 0x803B0DC (the same
 *     FUN_08035058(..., 349) call on the item == 0 branch) and pointed the
 *     table entries straight there.  The two calls are written separately in
 *     the source; the merge is the compiler's work.
 *
 *  5. THE INDEX COMPUTATION: the ROM has `subs r0,#36 / subs r0,r0,r1 /
 *     lsrs r2,r0,#3`.
 *     Subtracting the constant FIRST comes from gcc's
 *     `A - (B + constant)` -> `(A - constant) - B` folding; so the source is
 *     `((u32)node - (u32)slots) >> 3` (the slots array is at +0x24).  The
 *     `lsrs` is UNSIGNED, so a pointer difference (`node - slots`, ptrdiff_t)
 *     CANNOT BE USED -- that would emit `asrs`.
 *
 *  6. The block at 0x803B112 is BYTE-FOR-BYTE the same as GetActiveSlotValue
 *     (0x0803C090, src/world/slot_config.c) but there is no `bl`: the same
 *     selection is written out by hand in the source.  So it is written out
 *     here as well.
 *
 *  7. FUN_08035f1c TAKES ONE ARGUMENT (0x08035F1C never reads r1).  The r1
 *     before the call is the p + 0x1C value, a leftover from the 12-byte
 *     `ldmia/stmia` copy; there is NO second argument in the source.
 *
 *  8. `ldrh` (unsigned) => the +0x02 field is u16; `ldrsh` (0x803B16E) =>
 *     gSlotSelector is s16; the signed `blt/ble/bge` branches => the u8 fields
 *     promote to int, and the comparison is against int constants.
 *
 * ---------------------------------------------------------------------
 * THE SYMBOLS USED (all recorded in data/ram_map.csv)
 *   0x02000F08 gRam02000F08  — the context pointer (Ctx *)
 *   0x02000F04 gSessionPtr   — the slot pointer (Slot *)
 *   0x02000F10 gRam02000F10  — the primary slot body
 *   0x02000CE0 gGameState    — [12] the secondary slot selector
 *   0x02000D40 gSlotSelector — s16
 *   0x02000F00 gRam02000F00  — u8
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/band_0803afbc.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define QUERY_A   0x400C
#define QUERY_B   0x4009
#define QUERY_C   0x400B

#define NOTIFY_A  348
#define NOTIFY_B  349

#define STATE_READY  13

#define PHASE_ABORT  2
#define TIMER_RESET  900

typedef struct Vec3 {
    u32 x;
    u32 y;
    u32 z;
} Vec3;

/* the node at +0x1C */
typedef struct Node {
    u8  state;                  /* +0x00 */
    u8  pad01[1];
    u16 pending;                /* +0x02 */
} Node;

/* The target of the pointer at +0x20: the position is at +0x04 */
typedef struct AltBody {
    u32  pad00;
    Vec3 pos;                   /* +0x04 */
} AltBody;

/* Ctx->item (+0xB4) */
typedef struct Item {
    u8       pad00[8];
    u8       flags;             /* +0x08 */
    u8       pad09[15];
    Vec3    *pos;               /* +0x18 */
    u8       pad1c[4];
    AltBody *alt;               /* +0x20 */
} Item;

/* An element of the Ctx->slots array; stride 8 (the ROM: difference >> 3) */
typedef struct NodeSlot {
    u8 raw[8];
} NodeSlot;

typedef struct Ctx {
    u8       pad00[6];
    u16      timer;             /* +0x06 */
    u8       busy;              /* +0x08 */
    u8       pad09[2];
    u8       phase;             /* +0x0B */
    u8       pad0c[4];
    Vec3     pos;               /* +0x10 */
    Node    *node;              /* +0x1C */
    u8       pad20[4];
    NodeSlot slots[17];         /* +0x24 */
    u8       padac[2];
    u16      flags;             /* +0xAE */
    u8       padb0[4];
    Item    *item;              /* +0xB4 */
    u8       step;              /* +0xB8 */
    u8       padb9[3];
    u32      unkBC;             /* +0xBC */
} Ctx;

typedef struct Motion {
    u8  pad00[24];
    s32 speed;                  /* +0x18 */
} Motion;

typedef struct Entity {
    u8      pad00[24];
    Motion *motion;             /* +0x18 */
} Entity;

typedef struct Slot {
    Entity *entry;              /* +0x00 */
} Slot;

extern Ctx  *gRam02000F08;
extern Slot *gSessionPtr;
extern u8    gGameState[];
extern u8    gRam02000F00;
extern s16   gSlotSelector;

extern u32  ActorIdMatches(Entity *entry, u32 query);
extern void FUN_08035058(Entity *entry, u32 id);
extern void FUN_08035f1c(Entity *entry);
extern void FUN_0803a554(u32 index, s32 speed, u32 arg3);
extern void FUN_08030a3c(u16 value);
extern u32  GetRamType(void);

/* 0x0803AFBC */
void FUN_0803afbc(void)
{
    Node    *node;
    Item    *item;
    Vec3    *src;
    Entity  *entry;
    u32      index;
    s32      speed;
    int      limit;

    node = gRam02000F08->node;
    if ((gRam02000F08->flags & 2) != 0)
        return;
    if (gRam02000F08->busy != 0)
        return;
    if (ActorIdMatches(gSessionPtr->entry, QUERY_A) != 0)
        return;
    if (ActorIdMatches(gSessionPtr->entry, QUERY_B) != 0)
        return;
    if (ActorIdMatches(gSessionPtr->entry, QUERY_C) != 0)
        return;

    switch (gRam02000F08->node->state) {
    case 10:
        limit = 24;
        break;
    case 8:
    case 9:
        limit = 18;
        break;
    default:
        goto ready;
    }

    if (gRam02000F08->step != 0 && gRam02000F08->step < limit)
        return;

ready:
    if (node->pending == 0) {
        if (GetRamType() != 0)
            node->pending = 1;

        if (node->pending == 0) {
            switch (gRam02000F08->node->state) {
            case 3:
            case 5:
                FUN_08035058(gSessionPtr->entry, NOTIFY_B);
                return;
            case 4:
            case 6:
            case 7:
            case 10:
            case 11:
                FUN_08035058(gSessionPtr->entry, NOTIFY_A);
                return;
            }
            return;
        }
    }

    if (gRam02000F08->node->state == STATE_READY) {
        item = gRam02000F08->item;
        if (item == 0) {
            FUN_08035058(gSessionPtr->entry, NOTIFY_B);
            return;
        }

        if ((item->flags & 0x30) != 0)
            src = &item->alt->pos;
        else
            src = item->pos;

        gRam02000F08->pos = *src;

        if (gGameState[12] == 0)
            entry = ((Slot *)gRam02000F10)->entry;
        else
            entry = gSessionPtr->entry;

        FUN_08035f1c(entry);
    } else {
        gRam02000F08->phase = PHASE_ABORT;
        gRam02000F08->unkBC = 0;
        index = ((u32)gRam02000F08->node - (u32)gRam02000F08->slots) >> 3;
        speed = gSessionPtr->entry->motion->speed;
        if (speed < 0)
            speed = -speed;
        FUN_0803a554(index, speed, 0);
        if (gRam02000F00 == gSlotSelector + 1)
            FUN_08030a3c(node->pending);
        gRam02000F08->timer = TIMER_RESET;
        gRam02000F08->step = 1;
    }
}
