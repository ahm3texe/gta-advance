/* Fill the record table from the source list — 0x0800DCB4-0x0800DD4D
 *
 * The table object at 0x02015650 holds a 16x16 record array (an entry is 12
 * bytes, a row 192 bytes, the array 0x24..0xC24 = 0xC00). Two counters sit
 * immediately after the array: +0xC24 the outer counter and +0xC28 the inner
 * one. The function walks straight over 8-byte source records and writes each
 * into the table IN COLUMN ORDER: the outer counter scans the column (i) and
 * the inner counter the row (j), with the destination address being
 * table + i*12 + j*192. That is exactly the ROM's address generation
 * (lsl#1 + add + lsl#2, then +0xC0 on every step).
 *
 * If a source record's count is zero, all three words of the entry are cleared;
 * otherwise the count, the second field and an advancing pointer into the
 * 0x02016290 pool are written. The pool pointer advances by count WORDS per
 * record (lsls #2). At the end the table object's first word is cleared.
 *
 * 0x02016290 is the very 18400-byte block the sibling file
 * (src/core/list_b1.c) clears with DMA; the ROM loads it as a SEPARATE
 * literal rather than as 0x02015650 + 0xC40 -- which is why it was written as
 * a separate object (the same reasoning as in the sibling).
 *
 * WHY THE BASE IS extern (rule 1): the ROM reads 0x02015650 from the pool and
 * adds 0xC24 to it from a SEPARATE literal. With a raw address cast, agbcc
 * would fold 0x02016274 into a single literal and these three instructions
 * would disappear. Because the pool (0x02016290) is used without an offset, a
 * cast macro is enough there: both forms produce a single `ldr rX,[pc,...]`.
 *
 * Rule 9/31: all four comparisons are signed (bge/blt), so the counters and
 * the two fields are s32. Rule 35: `pop {r0}; bx r0` -> a void return type.
 * `src->count` is re-read in three separate places (0x800DCF4, 0x800DD14,
 * 0x800DD1E): the stores in between kill the load in agbcc, so the field is
 * read every time in the source too, without taking it into a local.
 *
 * WHAT WAS TRIED (do not repeat):
 *   - Advancing the parameter DIRECTLY (`void BuildEntryTableRows(SourceEntry *src)`
 *     + `src++`): 154/154 in size, the only difference being the position of
 *     the parameter copy. agbcc puts the parameter->pseudo copy in the
 *     prologue, so `adds r3,r0,#0` came out BEFORE the pool literal; in the
 *     ROM it comes after.
 *     The fix: leave the parameter read-only and build the walker as a
 *     SEPARATE local with the statement `src = entries;` -- the copy is now a
 *     body statement and takes its place after the pool assignment. 4 bytes ->
 *     0. (The rule 11/22 family: the POSITION of the assignment decides.)
 *   - Initialising `pool` in its declaration (`u32 *pool = gPool02016290;`)
 *     changed nothing on its own; the order difference did not come from
 *     there. It gives the same result together with the separate local too,
 *     but for readability both assignments were left in the body, in the ROM's
 *     order (rule 19).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/list_b2.c
 *
 * REQUIRED: a gRam02015650 record for 0x02015650 in data/ram_map.csv
 * (3116 bytes: 0x00..0xC2C). This file does not add the record itself.
 */

#include "gba_types.h"

/* The word pool the records' data pointers point into. */
#define gPool02016290  ((u32 *)0x02016290)

/* A single record in the table: 12 bytes. */
typedef struct {
    u32 count;                  /* 0x00 -- the 16-bit count from the source */
    u32 unk04;                  /* 0x04 -- copied verbatim from the source */
    u32 *data;                  /* 0x08 -- the slice inside the pool */
} TableEntry;

/* A source record: 8 bytes, walked as a flat array. */
typedef struct {
    u16 count;                  /* 0x00 */
    u16 pad02;
    u32 unk04;                  /* 0x04 */
} SourceEntry;

/* The table object at 0x02015650. */
typedef struct {
    u32 unk00;                  /* 0x00 -- cleared at the end */
    u8  pad04[0x20];
    TableEntry rows[16][16];    /* 0x24 -- a row is 192 bytes, 0xC00 total */
    s32 columnCount;            /* 0xC24 -- outer loop bound */
    s32 rowCount;               /* 0xC28 -- the inner loop bound */
} EntryTable;

extern EntryTable gRam02015650;

/* 0x0800DCB4 */
void BuildEntryTableRows(SourceEntry *entries)
{
    u32 *pool;
    SourceEntry *src;
    s32 i;
    s32 j;

    pool = gPool02016290;
    src = entries;

    for (i = 0; i < gRam02015650.columnCount; i++) {
        for (j = 0; j < gRam02015650.rowCount; j++) {
            TableEntry *entry = &gRam02015650.rows[j][i];

            if (src->count == 0) {
                entry->count = 0;
                entry->unk04 = 0;
                entry->data = 0;
            } else {
                entry->count = src->count;
                entry->unk04 = src->unk04;
                entry->data = pool;
            }

            pool += src->count;
            src++;
        }
    }

    gRam02015650.unk00 = 0;
}
