/* Is the attached record missing or of kind 6 — 0x0801D920-0x0801D939
 *
 * Answers 1 for a null object, a null record at +0x18, or a record whose +0x30
 * kind byte is 6; 0 otherwise. The placeholder name is kept: the callers were
 * not examined.
 *
 * The +0x30 read is `adds r0,#48 / ldrb r0,[r0,#0]` rather than a single
 * `ldrb r0,[r0,#48]` because Thumb's ldrb immediate reaches only 31; that
 * falls out of the natural field access and is not something to write by hand.
 *
 * Rule 49: the three answers of 1 share one body at the end of the function.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/kind_six_or_empty.c
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

/* 0x0801D920 */
u32 FUN_0801d920(KindOwner *owner)
{
    KindRecord *record;

    if (owner == 0) goto yes;
    record = owner->record;
    if (record == 0) goto yes;
    if (record->kind == KIND_SIX) goto yes;
    return 0;
yes:
    return 1;
}
