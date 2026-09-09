/* Counts the area records by a two-byte key.
 * PARKED: an extern/macro/volatile base and Ghidra's local order were tried.
 * The body is 124/124 with 95 bytes differing. When the compiler keeps the area
 * bank base in r7 it spills the first parameter to the stack; the ROM reloads
 * the base after the call. */

#include "gba_types.h"

typedef struct AreaEntry {
    u8 pad00[0x18];
    u8 kind;
    u8 variant;
    u8 pad1A[2];
} AreaEntry;

typedef struct AreaBankCount {
    u8 pad00[8];
    s32 count;
    u8 pad0C[0x14];
    AreaEntry *entries;
} AreaBankCount;

#define gAreaBank (*(AreaBankCount *)0x08D49C00)
extern s32 FUN_08031ff0(AreaEntry *entry);
extern s32 IsEntityFlagSet(AreaEntry *entry);

/* 0x080563C0 */
s32 FUN_080563c0(u32 kind, u32 variant, s32 *totalOut)
{
    s32 callResult;
    AreaEntry *entry;
    s32 offset;
    s32 index;
    s32 flagged;
    s32 total;

    index = 0;
    flagged = 0;
    total = 0;
    if (gAreaBank.count > 0) {
        offset = 0;
        do {
            entry = (AreaEntry *)((u8 *)gAreaBank.entries + offset);
            if (entry->kind == kind && entry->variant == variant) {
                callResult = FUN_08031ff0(entry);
                if (callResult >= 0) {
                total++;
                if (IsEntityFlagSet(entry) != 0)
                    flagged++;
                }
            }
            offset += sizeof(AreaEntry);
            index++;
        } while (index < gAreaBank.count);
    }
    if (totalOut != 0)
        *totalOut = total;
    return flagged;
}
