/* Build a velocity vector from the sine table and advance the actor -- 0x08025340-0x08025423
 *
 * 228 bytes, Thumb.  It does two independent things:
 *
 *  1. If the fourth argument (off) is not NULL and at least one of the two
 *     signed byte offsets inside it is nonzero, it rotates that offset by the
 *     actor's current angle (FUN_08029088), scales the result by 41/32 and ADDS
 *     it to the 16.16 position (+0x4C / +0x50).  GetOwnerSlot(actor->owner) is
 *     called in between; its return value is UNUSED, because r0 is overwritten
 *     immediately.
 *  2. It derives a 10-bit table index from angle + turn, reads sin and cos from
 *     the 1024-entry s16 sine table at 0x08CA30D8, multiplies them by four and
 *     writes them to +0x70 / +0x74.  If speed is nonzero it multiplies the same
 *     vector by speed and again adds it to +0x4C / +0x50.
 *
 * THE SIBLING: src/world/entries_b1.c is NOT from the same family -- its body is
 * something else entirely (slot searching).  For the struct layout and the call
 * signatures the real sibling is src/world/state_offset.c (0x080260A8): the
 * Actor view there uses exactly the same offsets (+0x4C px, +0x50 py, +0x68
 * angle, +0x84 owner) and FUN_08029088's signature was verified from the ROM
 * there.  This file additionally expands +0x70 and +0x74 (the velocity vector).
 *
 * DETAILS MEASURED FROM THE ROM
 *
 *  - The return type is void (rule 35): the epilogue is `pop {r0}; bx r0` and
 *    r0 is dead.
 *  - Argument types: r1 (turn) and r2 (speed) are never narrowed, both raw
 *    words -> s32.  r3 (off) is tested against zero -> a pointer.
 *  - The table at 0x08CA30D8 was read from the ROM: 1024 entries, amplitude
 *    16384, [0]=0, [256]=16384, [512]=0 -> a Q14 sine.  For cos the index is
 *    +256.
 *    The address is NOT in data/ram_map.csv and I have no authority to write
 *    there; hence it was written as a constant cast (the same reasoning as in
 *    entries_b2.c: the address is used purely as a base, so rule 1's
 *    base+offset folding does not arise here -- the ROM also reads it from the
 *    pool as a single word).
 *  - The 41/32 scale was read from the shift chain: lsls#2 / adds / lsls#3 /
 *    adds = x*41, then asrs#5.  The same as SCALE_NUM/SCALE_SH in
 *    state_offset.c.
 *  - The ox/oy stack slots are sp+4 and sp+5, i.e. two ADJACENT bytes;
 *    FUN_08029088 takes sp+4 as its fourth argument and sp+5 as its fifth.
 *
 * FORMS TRIED AND ELIMINATED (the most valuable part of this file)
 *
 *  1. `if (off->dx != 0 || off->dy != 0)` -- agbcc (GCC 2.8.1 fold_truthop)
 *     merges the zero comparison of two ADJACENT fields into a single
 *     `ldrh r0,[r3,#32]`.  The output is 224 bytes against the ROM's 228:
 *     exactly 2 instructions short (the ROM's raw byte copy `adds r2,r0,#0` and
 *     the fourth branch instruction).
 *     SEPARATING the field types as s8/u8 did not prevent the merge either
 *     (measured: s8+u8 with direct access still gives 224).  A bitfield
 *     (`signed char dx:8`), and making both bitfields, also stayed at 224.
 *  2. Reading the fields into locals first (`rx = off->dx; ry = off->dy;`)
 *     prevents the merge, but then agbcc derives the two addresses from each
 *     other: `adds r0,r3,#33` followed by `subs r0,#1`.  The ROM builds both
 *     SEPARATELY from r3 (`adds r1,r3,#0; adds r1,#32` +
 *     `adds r0,r3,#0; adds r0,#33`).
 *     The best result with that form was a 34-byte difference.
 *  3. The solution is rule 17/37: A SEPARATE POINTER LOCAL FOR EACH FIELD.
 *     Once `px` and `py` create two independent address lifetimes, agbcc
 *     rebuilds both from the base and the ROM's sequence comes out exactly.
 *  4. `sx` (the sign-extended dx) and `ry` (dy's RAW byte) must have DIFFERENT
 *     types.  The ROM sign-extends dx with lsls#24/asrs#24 and tests the
 *     EXTENDED value, while it tests dy with the RAW byte and defers the
 *     extension to the call setup (0x8025372).  Writing `s32 ry` sends the
 *     difference up to 188; `u8 ry` with `(s8)ry` in the argument is the right
 *     form.
 *     Skipping the `sx`/`ry` intermediates entirely and writing `*px`/`*py`
 *     directly also breaks it (192 differences).
 *  5. The tail (the sine table) needed THREE separate measurements:
 *       - `gSinTable[...]` directly: the mask literal enters the pool FIRST,
 *         while in the ROM the TABLE address comes first.  51 differences.
 *       - `tbl = gSinTable;` at the very top: the literal order is corrected
 *         but the `ldr` comes out BEFORE the angle computation.  47
 *         differences.
 *       - Splitting the angle into an `ang` local and placing the
 *         `tbl = gSinTable;` assignment IMMEDIATELY AFTER it (rule 16: not the
 *         declaration site but the ASSIGNMENT site): the tail matched exactly.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_b4.c
 */

#include "gba_types.h"

#define ANGLE_ROUND  0x8000     /* half a turn, to round the 16.16 angle */
#define TABLE_MASK   0x3FF      /* a 1024-entry table */
#define QUARTER      256        /* a quarter turn = the index shift for cos */
#define SCALE_NUM    41         /* 41/32 ~ 1.28, the same as state_offset.c */
#define SCALE_SH     5

/* 0x08CA30D8: a 1024-entry Q14 sine table (read from the ROM, see the header).
 * It has no entry in data/ram_map.csv; written as a constant cast. */
#define gSinTable    ((s16 *)0x08CA30D8)

/* The only part of the fourth argument that is touched: the signed byte
 * offset pair at +0x20 / +0x21.  The rest is unknown and left as padding. */
typedef struct Offsets {
    u8  pad00[0x20];
    s8  dx;              /* +0x20 */
    s8  dy;              /* +0x21 */
} Offsets;

/* The same offsets as the Actor view in src/world/state_offset.c; the velocity
 * vector (+0x70/+0x74) is expanded here.  The contents of owner are not read in
 * this translation unit; it is only passed to GetOwnerSlot. */
typedef struct Actor {
    u8    pad00[0x4c];
    s32   px;            /* +0x4C, 16.16 */
    s32   py;            /* +0x50, 16.16 */
    u8    pad54[0x14];
    s32   angle;         /* +0x68, 16.16 */
    u8    pad6c[4];
    s32   vx;            /* +0x70 */
    s32   vy;            /* +0x74 */
    u8    pad78[0xc];
    void *owner;         /* +0x84 */
} Actor;

/* The signature was confirmed from the ROM in state_offset.c. */
extern void FUN_08029088(s32 angle, s32 dx, s32 dy, s8 *outX, s8 *outY);
/* The return value is unused here (r0 is overwritten right after the call). */
extern void GetOwnerSlot(void *owner);

/* 0x08025340 */
void MoveActorAlongAngle(Actor *actor, s32 turn, s32 speed, Offsets *off)
{
    s32  idx;
    s32  vx;
    s32  vy;
    s8   ox;
    s8   oy;
    s16 *tbl;
    s32  ang;
    s8  *px;
    s8  *py;
    s32  sx;
    u8   ry;

    if (off != 0) {
        /* Rule 17/37: TWO SEPARATE pointer locals for the two fields;
         * accessing through a single base makes agbcc derive the addresses from
         * each other. */
        px = &off->dx;
        py = &off->dy;
        ry = (u8)*py;
        sx = *px;
        if (sx != 0 || ry != 0) {
            FUN_08029088((actor->angle + ANGLE_ROUND) >> 16, sx, (s8)ry,
                         &ox, &oy);
            GetOwnerSlot(actor->owner);
            ox = (ox * SCALE_NUM) >> SCALE_SH;
            oy = (oy * SCALE_NUM) >> SCALE_SH;
            actor->px += ox << 16;
            actor->py += oy << 16;
        }
    }

    ang = (actor->angle + turn + ANGLE_ROUND) >> 16;
    tbl = gSinTable;                    /* rule 16: where it is assigned matters */
    idx = ang & TABLE_MASK;
    vx = tbl[idx] << 2;
    vy = -(tbl[(idx + QUARTER) & TABLE_MASK] << 2);
    actor->vx = vx;
    actor->vy = vy;
    if (speed != 0) {
        actor->px += vx * speed;
        actor->py += vy * speed;
    }
}
