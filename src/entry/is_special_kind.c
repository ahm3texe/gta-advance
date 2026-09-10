/* Is either kind one of three — 0x08023660-0x0802368B
 *
 * Two fields are checked against the same three values, 0x4023, 24 and 108: the
 * +0x28 word of the record at +0x1C, and that record's +0x06 halfword. Either
 * matching answers 1.
 *
 * 0x4023 comes through the literal pool and is loaded ONCE for both of its
 * comparisons, which is what a single local holding it gives. That local is
 * assigned AFTER the first field is read, because the ROM loads the constant
 * after it; taken at the top the pool load comes first.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/is_special_kind.c
 */

#include "gba_types.h"

#define KIND_A  0x4023
#define KIND_B  24
#define KIND_C  108

typedef struct KindRecord {
    u8  pad00[6];
    u16 alt;                    /* +0x06 */
    u8  pad08[0x20];
    u32 kind;                   /* +0x28 */
} KindRecord;

typedef struct KindHolder {
    u8          pad00[0x1C];
    KindRecord *record;         /* +0x1C */
} KindHolder;

/* 0x08023660 */
u32 FUN_08023660(KindHolder *holder)
{
    KindRecord *record = holder->record;
    u32 first;
    u32 kind;

    kind = record->kind;
    first = KIND_A;
    if (kind == first) goto yes;
    if (kind == KIND_B) goto yes;
    if (kind == KIND_C) goto yes;
    kind = record->alt;
    if (kind == first) goto yes;
    if (kind == KIND_B) goto yes;
    if (kind == KIND_C) goto yes;
    return 0;
yes:
    return 1;
}
