/* Tablo secici ve okuyucu — 0x0805E6C4-0x0805E6FF
 *
 * ROM'da 0x08EC46D4 adresinden itibaren 0x9A8 byte'lik kayitlardan olusan bir
 * tablo var. gTableIndex hangi kaydin kullanildigini secer; ucuncu fonksiyon
 * o kayittan u32 okur. Tablonun ne tuttugu henuz bilinmiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/misc/table_lookup.c
 */

#include "gba_types.h"

#define TABLE_INDEX_MAX  4
#define TABLE_RECORD_U32 (0x9A8 / 4)

extern u32 gTableIndex;
extern u32 gRecordTable[][TABLE_RECORD_U32];   /* 0x08EC46D4, salt okunur */

/* 0x0805E6C4 */
void SetTableIndex(u32 index)
{
    if (index <= TABLE_INDEX_MAX)
        gTableIndex = index;
}

/* 0x0805E6D4 */
u32 GetTableIndex(void)
{
    return gTableIndex;
}

/* 0x0805E6E0 */
u32 GetRecordWord(u32 offset)
{
    return gRecordTable[gTableIndex][offset];
}
