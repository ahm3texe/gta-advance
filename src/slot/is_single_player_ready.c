/* Is the single-player chain ready — 0x0804FAE8-0x0804FB17
 *
 * Answers 0 outright in two-player mode. Otherwise a missing argument, a
 * missing +0x1C record or a missing +0x3C sub-record all answer 1, and only a
 * sub-record whose +0x27 byte is exactly 1 answers 0.
 *
 * The two zero answers share one body at the END, and the 1 falls into it from
 * above -- so the two-player test and the +0x27 test branch to the same place.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/slot/is_single_player_ready.c
 */

#include "gba_types.h"

#define TWO_PLAYER  12          /* gGameState byte 12 */

typedef struct SubRecord {
    u8 pad00[0x27];
    u8 state;                   /* +0x27 */
} SubRecord;

typedef struct Record {
    u8         pad00[0x3C];
    SubRecord *sub;             /* +0x3C */
} Record;

typedef struct Holder {
    u8      pad00[0x1C];
    Record *record;             /* +0x1C */
} Holder;

extern u8 gGameState[];

/* 0x0804FAE8 */
u32 FUN_0804fae8(Holder *holder)
{
    Record *record;
    SubRecord *sub;

    if (gGameState[TWO_PLAYER] != 0) goto no;
    if (holder == 0) goto yes;
    record = holder->record;
    if (record == 0) goto yes;
    sub = record->sub;
    if (sub == 0) goto yes;
    if (sub->state == 1) goto no;
yes:
    return 1;
no:
    return 0;
}
