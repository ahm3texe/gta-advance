/* Kayit tablosu erisimcileri — 0x080514C8-0x08051513
 *
 * 0x08CAC248 adresinden itibaren 60 byte'lik kayitlar var; gRecordIndex
 * hangisinin kullanildigini secer. Ilk iki fonksiyon secili kayittan birer
 * alan okuyup 16.16 sabit noktaya cevirir, ucuncusu istenen kaydin adresini
 * dondurur. Alanlarin anlami henuz bilinmiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/misc/record_table.c
 */

#include "gba_types.h"

typedef struct {
    u32 unk00;
    u32 unk04;
    u32 unk08;
    u8  unk0C[48];
} Record;                      /* 60 byte */

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
    /* Yerel isaretci sart: dogrudan gRecords[i].unk04 yazilirsa agbcc +4'u
     * taban literaline katliyor, ROM ise yukleme ofsetinde birakiyor. */
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
