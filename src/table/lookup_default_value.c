/* The value the default handler's entry carries — 0x080556C0-0x080556FF
 *
 * Scans the owner's 20-byte entry table for one whose handler is the stub at
 * 0x0805AB14 and whose +0x06 halfword matches the operand, and answers that
 * entry's +0x08 halfword. Nothing matching answers 0, and so does an owner with
 * no table.
 *
 * The handler's address is compared with its THUMB BIT SET, which is how the
 * table stores it, so the `__thumb`-suffixed symbol is what the comparison
 * needs (tools/agbcc_build.py).
 *
 * The count is read once into a countdown and the pointer advanced by 20 each
 * turn; the ROM does not index. The guard ahead of the loop is a SIGNED `bge`,
 * so both sides are ints even though the count is a halfword field, and the
 * value it compares is the answer's own zero rather than a separate index.
 *
 * The operand is NOT narrowed on entry (`adds r4,r1,#0` alone), so it is a wide
 * parameter and the halfword field is what widens for the comparison.
 *
 * Rule 70 for the handler: through a local, its pool load lands before the
 * countdown is set up, which is where the ROM has it.
 *
 * Rule 71 for the null test, with its own `return 0` rather than a shared
 * result variable. Sharing one hoists the zero above the test and overwrites
 * it, which is rule 48's shape; the `goto` form of rule 73 puts the body first
 * instead. Both were measured.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/table/lookup_default_value.c
 */

#include "gba_types.h"

typedef struct TableEntry {
    void *handler;              /* +0x00 */
    u16   pad04;
    u16   kind;                 /* +0x06 */
    u16   value;                /* +0x08 */
    u8    pad0A[10];
} TableEntry;                   /* 20 bytes */

typedef struct EntryTable {
    u8          pad00[0x0E];
    u16         count;          /* +0x0E */
    u8          pad10[4];
    TableEntry *entries;        /* +0x14 */
} EntryTable;

typedef struct TableOwner {
    u8                 pad00[0x2C];
    struct TableOwner *next;    /* +0x2C */
} TableOwner;

extern void FUN_0805ab14__thumb(void);

/* 0x080556C0 */
u32 FUN_080556c0(TableOwner *owner, u32 kind)
{
    TableOwner *inner = owner->next;
    EntryTable *table;
    TableEntry *entry;
    void *handler;
    s32 left;
    s32 count;
    s32 result;

    if (inner == 0) {
        return 0;
    } else {
        result = 0;
        table = (EntryTable *)inner->next;
        entry = table->entries;
        count = table->count;
        if (result < count) {
            handler = (void *)FUN_0805ab14__thumb;
            left = count;
            do {
                if (entry->handler == handler) {
                    if (entry->kind == kind)
                        result = entry->value;
                }
                left--;
                entry++;
            } while (left != 0);
        }
        return result;
    }
}
