/* Session setup -- 0x08030E10-0x08030E77, 104 bytes.
 * STATUS: all 50 of the 50 instructions are THE SAME as the ROM's.  The one
 * remaining difference is the pool word of the 0x02026DF0 symbol, which is not
 * yet in data/ram_map.csv.  Once the row is added the difference becomes 0
 * (measured, below).
 *
 * If the EWRAM counter is set it prepares the working block at 0x02026DF0:
 * sets its flag to 1, copies the +0x14 word from the progress block, moves the
 * 36 bytes at the record buffer's +0x64 to the block's +0x60, and calls
 * FUN_0802fd48 with the block's +0x08 record (mode 1).  If gGameState[12] is
 * set it sets up the second record (+0x34, mode 2) as well.
 *
 * A RAM_MAP ROW IS NEEDED -- THAT IS THE ONLY THING MISSING:
 *     0x02026DF0, >=0x84 bytes, gRam02026DF0
 * The symbol is not in data/ram_map.csv; agbcc_build.py cannot resolve it and
 * sys.exits.  (This file does not touch data/*.csv; the project owner will add
 * the row.)
 *
 * THE MEASUREMENT (the evidence): the file was temporarily compiled with the
 * neighbouring symbol gRam02026E80 (0x02026E80), which IS already in ram_map
 * -- the code generation is the same, only the pool word changes:
 *     make c-match  -> different: 2/104 bytes
 *     diff_function -> 49/50 instructions the same, 1 different
 * The only "instruction" counted as different is the 0x02026DF0 pool word
 * itself (0x6E80 instead of 0x6DF0); the 2 real bytes are exactly the low half
 * of that word.  All 42 instructions of the body are identical to the ROM's.
 *
 * THE FORM READ FROM THE ROM (disassembly, not guesswork):
 *   - The entry test is an EARLY RETURN: `cmp #0 / bne forward / movs r0,#0 /
 *     b end`.  The zero return sits right after the test, so the source has a
 *     plain `if (...) return 0;` at the TOP of the function as well.  Rule 49
 *     (move the rare body to the end) DOES NOT APPLY HERE: the branch jumps
 *     forward and SKIPS the body; the body is not the later block.
 *   - The 36-byte copy is THREE ldmia/stmia pairs (`{r2,r3,r5}` with
 *     write-back).  Rule 32: that only comes out of a STRUCT ASSIGNMENT
 *     (`d->unk60 = s->unk64`); a `*d++ = *s++` triple would produce separate
 *     ldr/str.
 *   - Both bases are loaded from the pool and offset with `adds rN,#offset`;
 *     there is NO folded literal (0x02026E50 / 0x02000DB4) -> rule 1: both
 *     must be extern SYMBOLS, not `((T*)0xADDR)` casts.  A cast would fold
 *     base+offset into a separate pool word.
 *   - The two records passed are separate NAMED members; `Record record[2]` +
 *     a constant index was not tried, because a constant index folds anyway
 *     while a named member gives the ROM's `adds r0,r4,#0 / adds r0,#0x34`
 *     pair directly.
 *   - FUN_0802fd48's return value is unused; the signature was written `void`
 *     (in the direction of rule 35, it makes no difference at the call site).
 *
 * PATHS RULED OUT: none -- the first spelling gave the ROM exactly, no search
 * was needed.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/area_cleanup_b1.c
 *             (1/1 byte-matching expected once the ram_map row is added)
 */

#include "gba_types.h"
#include "session.h"
#include "ram_symbols.h"

/* The 36-byte group moved from the record buffer to the block's +0x60.  Its
 * contents are unknown; only its size was measured (three ldmia/stmia
 * pairs). */
/* The record set up by FUN_0802fd48: +0x00/+0x02 u16, +0x04 three words,
 * +0x10 u32, +0x14 a sub-record, +0x26 u16, +0x28 u8.  0x2C in total
 * (the gap between the block's two instances: 0x34 - 0x08). */
/* 0x02026DF0 -- the session working block. */
/* 0x02000D50 -- the record buffer; the layout comes from src/world/area_flags.c. */
typedef struct SaveBuffer {
    u8       header[12];        /* 0x00 */
    u8       unk0C[48];         /* 0x0C */
    u32      entityFlags[4];    /* 0x3C */
    u32      areaFlags[6];      /* 0x4C */
    Snapshot unk64;             /* 0x64 -- the copied group */
    u8       unk88[20];         /* 0x88 */
    u8       complement;        /* 0x9C */
    u8       unk9D[3];
} SaveBuffer;

/* 0x02025810 -- the progress block; its +0x14 word is moved into the block. */
typedef struct Progress {
    u8  pad0000[0x14];
    u32 unk14;                  /* 0x14 */
} Progress;

extern u32        gFrameCounterEwram;   /* 0x02000EB4 */
extern u8         gGameState[16];       /* 0x02000CE0 */
extern SaveBuffer gSaveBuffer;          /* 0x02000D50 */
extern Session    gRam02026DF0;         /* 0x02026DF0 -- a ram_map row is required */

extern void FUN_0802fd48(Record *record, s32 kind);

/* 0x08030E10 */
u32 CaptureSessionSnapshot(void)
{
    Session *session;

    if (gFrameCounterEwram == 0)
        return 0;

    session = &gRam02026DF0;
    session->unk00 = 1;
    session->unk04 = ((Progress *)gRam02025810)->unk14;
    session->unk60 = gSaveBuffer.unk64;
    FUN_0802fd48(&session->unk08, 1);

    if (gGameState[12] != 0)
        FUN_0802fd48(&session->unk34, 2);

    return 1;
}
