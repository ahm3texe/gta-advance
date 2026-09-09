/* Advancing the entry timer -- 0x08028D44-0x08028DC3
 *
 * MATCHED: 128/128 bytes, 0 off.  make c-match CLEAN, make c-review CLEAN.
 *
 * Computes the 148-byte entry from the index and resolves the target through a
 * two-level ROM table; if the entry is in the right state and its +0x90 field
 * is 3 it sets a pointer, calls FUN_08013CFC, decrements the timer by 4 and
 * returns 1 if it stays positive.
 *
 * THE PARAMETER TYPES were read from the instruction sequence: the second
 * parameter is SIGNED 16-bit (s16) via `lsls #16 / asrs #16`, the third
 * UNSIGNED 8-bit (u8) via `lsls #24 / lsrs #24`.  Writing a wider type removes
 * those truncating instructions.
 *
 * The timer test is `lsls #16` + `cmp <= 0`, i.e. the decremented halfword is
 * tested SIGNED.
 *
 * Rule 35: `pop {r1}; bx r1` -> r0 carries a return value, so the signature is
 * u32.
 *
 * THE TYPE WAS UNIFIED: gRam020246F0 was already defined as `Entry[20]` in
 * src/world/table_entries.c, and in that definition +0x02 (unk02) and +0x04
 * (unk04) were in the RIGHT place.  Using `&gRam020246F0[index]` instead of
 * computing `base + index*148` by hand is the right thing to do; giving the
 * same symbol two types broke the TYPES-001 gate.  The missing fields
 * (mark +0x2A, state +0x2B, tableIndex +0x64, phase +0x90) were carved out of
 * the padding and table_entries.c was PRESERVED at 3/3.
 *
 * ------------------------------------------------------------------
 * HOW THE LAST 4 BYTES CLOSED (124 -> 128, 55 off -> 0)
 * ------------------------------------------------------------------
 * An instruction-by-instruction diff showed that a SINGLE region (between the
 * mark write and the bl) diverged.  The ROM against our old output:
 *
 *   the ROM                     ours, before
 *   adds r0, r4, #0             adds r0, r4, #4     <- arg1 FIRST
 *   adds r0, #140               adds r1, #98        <- r1 was entry+0x2A
 *   ldr  r0, [r0, #0]           ldr  r1, [r1, #0]
 *   asrs r0, r0, #2             lsrs r1, r1, #2
 *   ldr  r1, [r3, #4]           ldr  r2, [r3, #4]
 *   lsls r0, r0, #2             lsls r1, r1, #2
 *   adds r0, r0, r1             adds r1, r1, r2
 *   ldr  r1, [r0, #0]           ldr  r1, [r1, #0]
 *   adds r0, r4, #4             (absent -- it had been done above)
 *
 * THE MECHANISM: with the arg2 chain produced inside the call setup, arg1
 * (`&entry->unk04`) went into r0 FIRST and r1 was left for the arg2 chain;
 * since r1 already carried `entry+0x2A` (the mark pointer) at that moment, the
 * compiler produced `entry+0x8C` in a SINGLE instruction with `adds r1, #98`.
 * That was the missing 2 bytes -- and the other 2 followed from it: with the
 * code 2 bytes shorter, the `movs r0,r0` (nop) padding before the pool
 * disappeared.
 *
 * THE SOLUTION: take arg2 into a SEPARATE STATEMENT (`src = ...;`).  That
 * makes the RTL order match the ROM's: the arg2 chain first (r0 nailed down,
 * 0xFF dead at that point), then `adds r0, r4, #4` in the call setup.  Because
 * r0 does not carry an address at that point, `entry+0x8C` is recomputed from
 * r4 -> the extra instruction we were looking for.
 *
 * The second piece: the unk8C field is s32 (SIGNED).  `>> 2` therefore
 * produces `asrs`.
 *
 * THE MEASURED CONTRIBUTIONS (tried separately):
 *   the baseline (u32 unk8C, arg2 inside the call)  -> 124 bytes, 55 off
 *   ONLY s32 unk8C                                  -> 124 bytes, 54 off (not enough)
 *   ONLY the separate `src` local                   -> 128 bytes,  1 off (asrs missing)
 *   BOTH TOGETHER                                   -> 128 bytes,  0 off  MATCHED
 * So the statement split fixes the size and s32 fixes the last byte.
 *
 * PATHS RULED OUT (from the earlier hand-computed version; DO NOT TRY THEM
 * AGAIN):
 *   an (s32) cast on the shift          -> 124 bytes
 *   making the first argument (u8*)entry+4 -> 124 bytes
 *   making the first argument entry->pad04 -> 124 bytes
 *   making the first argument &entry->unk02 + 1 -> 124 bytes
 * None of these helped, because the problem was not THE FORM OF ARG1 but WHEN
 * arg2 was produced.  Fiddling with the arg1 expression was the wrong axis.
 * THE LESSON: a difference that looks like "the wrong register" was really a
 * difference in EMISSION ORDER; taking a subexpression into a separate
 * statement (using rule 69 in the reverse direction) turns the production
 * order of the call arguments into the ROM's.
 *
 * A pool note (for the record): no pool word was ever missing.  Both pools
 * carried the same two constants (0x020246F0 and 0x08BD3448), only shifted by
 * 4 bytes; what was missing was the code BEFORE the pool.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/step_entry_timer.c
 */

#include "gba_types.h"

#define STATE_READY  1
#define PHASE_DONE   3
#define TIMER_STEP   4
#define MARK_VALUE   0xFF

#define ROM_TABLE ((TableA *)0x08BD3448)

typedef struct TableB {
    u8    pad00[4];
    u32 **slots;                /* +0x04 */
} TableB;

typedef struct TableA {
    u8       pad00[4];
    TableB **slots;             /* +0x04 */
} TableA;

typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01;
    u16 unk02;                  /* +0x02 */
    u32 unk04;                  /* +0x04 (the block to be released) */
    u8  pad08[0x22];
    u8  mark;                   /* +0x2A */
    u8  state;                  /* +0x2B */
    u8  pad2C[0x38];
    u8  tableIndex;             /* +0x64 */
    u8  pad65[0x27];
    s32 unk8C;                  /* +0x8C */
    u32 phase;                  /* +0x90 */
} Entry;

extern Entry gRam020246F0[20];

extern void FUN_08013cfc(void *dest, u32 *src, u32 arg);

/* 0x08028D44 */
u32 StepEntryTimer(u32 unused, s16 index, u8 arg)
{
    Entry *entry;
    TableB *b;
    u32 *target;
    u32 *src;
    u32 phase;

    entry = &gRam020246F0[index];
    b = ROM_TABLE->slots[entry->tableIndex];
    phase = entry->phase;
    target = (u32 *)b->slots[phase];

    if (entry->state != STATE_READY)
        return 0;
    if (phase != PHASE_DONE)
        return 1;

    entry->mark = MARK_VALUE;
    src = (u32 *)((TableB *)target)->slots[entry->unk8C >> 2];
    FUN_08013cfc(&entry->unk04, src, arg);

    entry->unk02 -= TIMER_STEP;
    if ((s16)entry->unk02 <= 0)
        return 0;
    return 1;
}
