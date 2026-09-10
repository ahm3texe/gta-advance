/* The record's +0x100 field, when its +0x114 byte is set — 0x0803F9DC-0x0803F9F9
 *
 * 0x114 is `movs r2,#138 / lsls r2,#1`, and GetField100 reads the +0x100 word
 * of the same record, which is 20 bytes earlier -- the same pair
 * src/script/cmd_owner_link_matches.c reads directly.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/actor/field100_if_flagged.c
 */

#include "gba_types.h"

#define FLAG_OFFSET  (138 << 1) /* 0x114 */

typedef struct FieldHolder {
    u32  pad00;
    u8  *record;                /* +0x04 */
} FieldHolder;

extern u32 GetField100(u8 *record);

/* 0x0803F9DC */
u32 FUN_0803f9dc(FieldHolder *holder)
{
    u8 *record = holder->record;

    if (record[FLAG_OFFSET] == 0)
        return 0;
    return GetField100(record);
}
