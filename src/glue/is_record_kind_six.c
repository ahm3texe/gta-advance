/* Is the record's +0x30 byte 6 — 0x0804FB18-0x0804FB29
 *
 * The +0x30 read is `adds r0,#48 / ldrb r0,[r0,#0]` because Thumb's ldrb
 * immediate reaches only 31; src/session/owner_ready.c reads the same field the
 * same way.
 *
 * Rule 73, and note which way round: the 1 is written AFTER the label, so the 0
 * is the one that falls through first. src/entity/is_lookup_nonnegative.c is
 * the mirror image of this and needs the arms the other way.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/is_record_kind_six.c
 */

#include "gba_types.h"

#define KIND_SIX  6

typedef struct KindRecord {
    u8 pad00[0x30];
    u8 kind;                    /* +0x30 */
} KindRecord;

typedef struct KindOwner {
    u8          pad00[0x18];
    KindRecord *record;         /* +0x18 */
} KindOwner;

/* 0x0804FB18 */
u32 FUN_0804fb18(KindOwner *owner)
{
    if (owner->record->kind != KIND_SIX) goto no;
    return 1;
no:
    return 0;
}
