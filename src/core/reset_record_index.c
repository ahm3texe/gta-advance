/* Reset the record index block — 0x08051300-0x0805131B
 *
 * Writes the four words of the 16-byte block at 0x020303D0: a marker, zero, a
 * countdown of 3600 and zero.
 *
 * The ROM builds 3600 as `movs r0,#225 / lsls r0,#4` rather than loading it
 * from the pool, which is what a plain constant gives (rule 53); the marker
 * 0x7E7E does not fit an immediate and comes from the pool.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/reset_record_index.c
 */

#include "gba_types.h"

#define RECORD_MARKER   0x7E7E
#define RECORD_TIMEOUT  3600

typedef struct RecordIndex {
    u32 marker;
    u32 unk04;
    u32 timeout;
    u32 unk0C;
} RecordIndex;

extern RecordIndex gRecordIndex;

/* 0x08051300 */
void ResetRecordIndex(void)
{
    gRecordIndex.marker = RECORD_MARKER;
    gRecordIndex.unk04 = 0;
    gRecordIndex.timeout = RECORD_TIMEOUT;
    gRecordIndex.unk0C = 0;
}
