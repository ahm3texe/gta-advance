/* Release the first entry above the threshold — 0x0805A9A8-0x0805AA19
 *
 * NOT BYTE-MATCHING. 48 of 53 instructions, same length, and the difference is
 * a pure register-allocation tie: the ROM gives the loop index r2 and the count
 * r3, agbcc the other way round. Every instruction is otherwise identical,
 * including both signed comparisons and the scan's shape.
 *
 * Seven spellings were measured -- the three locals in three declaration
 * orders, the count read before the index, a `while` in place of the
 * first-test-plus-do-while, and both with the halfword as u16 and as s32 -- and
 * the pair never swaps. Rule 50's priority is equal for the two (three
 * references each, overlapping lifetimes), so there is nothing in the source to
 * break the tie with.
 *
 * Scans the record's +0x0C halfword list for the first entry above 7776 and, if
 * one is found, builds the point for it, nudges it by the +0x20 and +0x21
 * signed bytes and releases the slot it maps to.
 *
 * The scan is written as a first test followed by a do-while, because that is
 * what the ROM has: the first halfword is loaded before the loop and the loop
 * re-tests at its bottom. A plain `while` over the whole thing loads it twice.
 *
 * The halfword the scan leaves behind is the one the tail uses, so a zero after
 * the scan means the list was empty rather than that nothing was above the
 * threshold -- the ROM tests it against zero, not against the count.
 *
 * The guard before the scan compares the ANSWER against the count, not an
 * index: the ROM's `cmp r1,r3` uses the same register the halfword later lands
 * in, so the zero it is initialised to is what gets compared.
 * src/table/lookup_default_value.c has the same idiom.
 *
 * The comparisons against the threshold are SIGNED (`ble`/`bgt`), which is why
 * the halfword goes into an `s32` rather than staying a `u16`.
 *
 * 7776 is `movs r4,#243 / lsls r4,#5`, kept in a callee-saved register across
 * the whole scan.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_release_first_above.c
 */

#include "gba_types.h"

#define THRESHOLD   (243 << 5)  /* 7776 */
#define UNIT_SHIFT  16

typedef struct EntryRecord {
    u8   pad00[5];
    u8   count;                 /* +0x05 */
    u8   pad06[6];
    u16 *entries;               /* +0x0C */
    u8   pad10[0x10];
    s8   offsetX;               /* +0x20 */
    s8   offsetY;               /* +0x21 */
} EntryRecord;

typedef struct EntryNode {
    u8           pad00[0x14];
    EntryRecord *record;        /* +0x14 */
} EntryNode;

typedef struct SpawnPoint {
    s32 x;
    s32 y;
    s32 z;
} SpawnPoint;

extern EntryNode *FindOrRecycleNode(s32 id);

extern u32  FUN_080557ac(SpawnPoint *point, u16 id);
extern u32  FUN_0803095c(s32 x);
extern void ReleaseSlot(u32 index);

/* 0x0805A9A8 */
u32 FUN_0805a9a8(u32 a, u16 id)
{
    EntryNode *node = FindOrRecycleNode(id);
    EntryRecord *record = node->record;
    SpawnPoint point;
    s32 value;
    s32 index;
    s32 count;
    u16 *entry;

    value = 0;
    index = 0;
    count = record->count;
    if (value < count) {
        entry = record->entries;
        value = *entry;
        if (value > THRESHOLD) {
            do {
                entry++;
                index++;
                if (index >= count)
                    goto scanned;
                value = *entry;
            } while (value > THRESHOLD);
        }
    }
scanned:
    if (value == 0)
        return 1;
    if (FUN_080557ac(&point, value) == 0)
        return 1;
    point.x += record->offsetX << UNIT_SHIFT;
    point.y += record->offsetY << UNIT_SHIFT;
    ReleaseSlot(FUN_0803095c(point.x));
    return 1;
}
