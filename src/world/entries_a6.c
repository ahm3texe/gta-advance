/* Advance the entry animation by one frame -- 0x080289BC-0x08028A87
 *
 * Advances the animation position of a gEntriesA entry (148 bytes; sibling
 * src/world/entries_a5.c). The flow:
 *
 *   node  = gRom08BD3448.slots[e->kind]->slots[e->phase]
 *   limit = node->count << 16              (the last frame in 16.16 fixed point)
 *   if the position has reached limit:
 *       phase != 51 -> the sub-object is released with FUN_08013ABC(e->sub),
 *                      e->active = 0 and it RETURNS
 *       phase == 51 -> the position is rewound to limit - 0x20000 (two frames
 *                      back)
 *   frame = node->slots[the UPPER halfword of the position]
 *   FUN_08013CFC(e->sub, frame, 0);  FUN_08014EE4(e->sub, frame->unk14);
 *   if phase == 51 the position += 0x5000, and on overflow (past 0x4FFFF)
 *       FUN_08035230(GetActiveSlot(), 469) is called, and if the owner object's
 *       +0x0A flag is 2 the 0x8000 flag is added to +0x0C
 *   otherwise the position += 0x8000
 *
 * DETAILS MEASURED FROM THE ROM
 * -----------------------------
 * Rule 35: `pop {r4,r5,r6,r7}; pop {r0}; bx r0` -> a void return type.
 *
 * THE POSITION FIELD (+0x8C) IS A UNION. The ROM reads the same four bytes at
 * two widths: as a word (`ldr r0,[r7,#0]`, `str`) and as a SIGNED upper
 * halfword (`movs r1,#2; ldrsh r0,[r7,r1]`). In Thumb, LDRSH only exists in the
 * register-offset form, so the base is built as `e+0x8C` with index 2.
 * Writing `position >> 16` would emit `asrs` -- the ROM has none, so the source
 * really does have a separate s16 field.
 *
 * THE TWO COMPARISONS DIFFER IN SIGNEDNESS, and that fixed the types:
 *   0x80289EA `bcc`  -> UNSIGNED : `position >= limit`, with limit a `u32` local
 *   0x8028A42 `ble`  -> SIGNED   : `position > 0x4FFFF`, with the field `s32`
 * So the field is s32; what makes the upper-bound comparison unsigned is that
 * `limit` is u32. Making limit `int` emits `blt`, and making the field u32
 * makes the second branch `bls` -- both wrong.
 *
 * PHASE IS READ THREE TIMES BUT THERE ARE TWO LOADS. The first two uses (the
 * array index and `cmp #51`) come from a single `ldr`, while the third
 * (0x8028A2E) is reloaded AFTER the calls. Writing `e->phase` everywhere in the
 * source produces exactly this: agbcc's CSE invalidates the memory read at a
 * call boundary. Keeping a separate `phase` local would move the value into a
 * callee-saved register and demand an extra push.
 *
 * Rule 33 (the mask constant in its own register): the ROM emits `movs r0,#2`
 * BEFORE `ldrh r1,[r5,#10]` and keeps the result in the constant's register.
 * Writing `if ((owner->flags & 2) != 0)` moves the ldrh to the front;
 * `mask = 2; mask &= owner->flags;` gives the ROM's order.
 *
 * A confirmation of rule 49: in the ROM the "finish" body (FUN_08013ABC +
 * active=0) sits on the STRAIGHT branch of the condition, while the rewind body
 * sits after the pool and FALLS THROUGH to L_a06. That is the natural layout of
 * the plain form `if (...) { if (phase != 51) { ...; return; } rewind; }`; no
 * goto or label was needed.
 *
 * This function does NOT touch gEntriesA at all (it takes the entry as a
 * parameter), so the table symbol was not declared.
 *
 * HOW THE LAST 8 BYTES CLOSED (212 -> 204, 100 -> 0 differences)
 * -------------------------------------------------------------
 * The first version wrote `e->pos...` everywhere. The body was instruction for
 * instruction the ROM's, but the prologue came out as `push {r4,r5,r6,lr}`
 * while the ROM's is `push {r4,r5,r6,r7,lr}`. So the ROM has ONE MORE LIVE
 * VALUE (the register table in docs/COMPILER.md): the ROM holds the `e+0x8C`
 * address in r7 and reuses it THREE TIMES (the `ldrsh`, the phase-51 step and
 * the default step), whereas ours rebuilt it each time with
 * `adds r0,r6,#0 / adds r0,#140` -- three extra instructions in total, 8 bytes.
 *
 * THE SOLUTION: introducing the local `Pos *pos = &e->pos;`. A textbook example
 * of rule 11/16: what matters is not the declaration but the ASSIGNMENT SITE.
 * Put at the top of the function, the assignment lengthens the lifetime and the
 * FIRST block uses r7 as well; the ROM instead builds the address in scratch r2
 * in the first block and REBUILDS it into r7 at L_a06. That is why the
 * assignment sits exactly AFTER the rewind if, immediately before the first
 * `ldrsh` use -- and the ROM's `adds r7,r6,#0 / adds r7,#140` pair lands there.
 * Because the first block comes before the local, it still writes
 * `e->pos.value` directly; the two forms are mixed deliberately.
 *
 * TRIED AND ELIMINATED
 * --------------------
 * - `e->pos.value` everywhere (no local at all) -> 212 bytes, 100 differences.
 *   The body is correct; the only thing missing is r7 staying live.
 * - Assigning the `pos` local at the top of the function was NOT TRIED, but
 *   under rule 16 it would make the first block use r7 instead of r2; in the
 *   ROM the first block uses r2, so that path is CONTRARY to the ROM.
 * - `position >> 16` for the frame index: the ROM has no `asrs`, it has
 *   `ldrsh`; hence the union field was written and the shift was not tried.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_a6.c
 */

#include "gba_types.h"

#define PHASE_SPECIAL   51          /* the separate +0x90 == 51 branch */
#define REWIND_STEP     0x20000     /* two frames when rewinding */
#define STEP_SPECIAL    0x5000      /* the phase 51 frames step */
#define STEP_DEFAULT    0x8000      /* the frame step in the other phases */
#define END_LIMIT       0x0004FFFF  /* the overflow threshold in phase 51 */
#define NOTIFY_ID       469         /* FUN_08035230's second argument */
#define OWNER_FLAG      2           /* the bit tested inside +0x0A */
#define OWNER_SET_BIT   0x8000      /* the bit added to +0x0C */

/* The ROM root at 0x08BD3448 and the nodes chained from it; the same layout as
 * RomNode in src/world/entries_b1.c. The difference: this translation unit
 * reads +0x00 as a BYTE (`ldrb`), where it was padding. */
typedef struct RomNode {
    u8              count;      /* +0x00, the frame count */
    u8              pad01[3];
    struct RomNode **slots;     /* +0x04 */
    u8              pad08[12];
    u32             unk14;      /* +0x14 */
} RomNode;

extern RomNode gRom08BD3448;

/* +0x8C: the 16.16 fixed-point animation position. Read both as a word and as
 * a SIGNED upper halfword. */
typedef struct PosHalf {
    u16 frac;                   /* +0x00 */
    s16 frame;                  /* +0x02 */
} PosHalf;

typedef union Pos {
    s32     value;
    PosHalf half;
} Pos;

/* The owner object at +0x84; only two of its fields are read. */
typedef struct Owner {
    u8  pad00[10];
    u16 flags;                  /* +0x0A */
    u32 bits;                   /* +0x0C */
} Owner;

/* The same layout as the sibling files (entries_a1.c ... entries_a5.c,
 * entries_b1.c); the fields this function touches were expanded. 148 = 0x94 in
 * total. */
typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     pad01[3];
    u8     sub[38];             /* +0x04, passed to FUN_08013CFC/FUN_08014EE4 */
    u8     pad2a[58];
    u8     kind;                /* +0x64 */
    u8     pad65[31];
    Owner *owner;               /* +0x84 */
    u8     pad88[4];
    Pos    pos;                 /* +0x8C */
    u32    phase;               /* +0x90, stride 148 */
} Entry;

extern void ReleaseObject(u8 *sub);
extern void FUN_08013cfc(u8 *dest, RomNode *desc, u32 arg);
extern void FUN_08014ee4(u8 *dest, u32 value);
extern u32 GetActiveSlot(void);
extern void FUN_08035230(u32 slot, u32 id);

/* 0x080289BC */
void AdvanceEntryFrame(Entry *e)
{
    RomNode *node;
    RomNode *frame;
    Owner *owner;
    Pos *pos;
    u32 limit;
    u32 mask;

    node = gRom08BD3448.slots[e->kind]->slots[e->phase];
    limit = node->count << 16;

    if (e->pos.value >= limit) {
        if (e->phase != PHASE_SPECIAL) {
            ReleaseObject(e->sub);
            e->active = 0;
            return;
        }
        e->pos.value = limit - REWIND_STEP;
    }

    pos = &e->pos;
    frame = node->slots[pos->half.frame];
    FUN_08013cfc(e->sub, frame, 0);
    FUN_08014ee4(e->sub, frame->unk14);

    if (e->phase == PHASE_SPECIAL) {
        pos->value += STEP_SPECIAL;
        if (pos->value > END_LIMIT) {
            FUN_08035230(GetActiveSlot(), NOTIFY_ID);
            owner = e->owner;
            mask = OWNER_FLAG;
            mask &= owner->flags;
            if (mask != 0)
                owner->bits |= OWNER_SET_BIT;
        }
    } else {
        pos->value += STEP_DEFAULT;
    }
}
