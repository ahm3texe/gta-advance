/* Record table accessors — 0x080514C8-0x08051513
 *
 * From 0x08CAC248 onwards there are 60-byte records; gRecordIndex selects
 * which one is in use. The first two functions each read one field from the
 * selected record and convert it to 16.16 fixed point; the third returns the
 * address of the requested record. The meaning of the fields is not known yet.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/misc/record_table.c
 */

#include "gba_types.h"

typedef struct {
    u32 unk00;
    u32 unk04;
    u32 unk08;
    u8  unk0C[48];
} Record;                      /* 60 bytes */

typedef struct RecordBlock {
    u32 index;                  /* +0x00 */
    u8  pad04[8];
    u32 state;                  /* +0x0C */
} RecordBlock;

extern RecordBlock gRecordIndex;
extern Record gRecords[];

/* 0x080514C8 */
u32 GetRecordUnk04(void)
{
    /* A local pointer is required: writing gRecords[i].unk04 directly makes
     * agbcc fold the +4 into the base literal, whereas the ROM leaves it in
     * the load offset. */
    Record *record = &gRecords[gRecordIndex.index];

    return record->unk04 << 16;
}

/* 0x080514E4 */
u32 GetRecordUnk08(void)
{
    Record *record = &gRecords[gRecordIndex.index];

    return record->unk08 << 16;
}

/* 0x08051500 */
Record *GetRecord(u32 index)
{
    return &gRecords[index];
}
