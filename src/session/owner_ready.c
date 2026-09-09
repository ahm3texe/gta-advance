/* Is the owner ready — 0x08061FBC-0x08061FDD
 *
 * Answers 1 when the record at +0x18 has a non-zero +0x32 byte, and otherwise
 * falls back on FindEntryByOwnerState. Only when both say no is the answer 0.
 *
 * The +0x32 read is `adds r0,#50 / ldrb r0,[r0,#0]` because Thumb's ldrb
 * immediate reaches only 31; that falls out of the field access itself.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/session/owner_ready.c
 */

#include "gba_types.h"

typedef struct ReadyRecord {
    u8 pad00[0x32];
    u8 ready;                   /* +0x32 */
} ReadyRecord;

typedef struct ReadyOwner {
    u8           pad00[0x18];
    ReadyRecord *record;        /* +0x18 */
} ReadyOwner;

extern u32 FindEntryByOwnerState(ReadyOwner *owner);

/* 0x08061FBC */
u32 FUN_08061fbc(ReadyOwner *owner)
{
    if (owner->record->ready != 0) goto yes;
    if (FindEntryByOwnerState(owner) != 0) goto yes;
    return 0;
yes:
    return 1;
}
