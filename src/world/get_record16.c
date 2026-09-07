/* 16 baytlik kayit tablosundan giris — 0x0800DB60-0x0800DB7B
 *
 * gRam0201AA98 tablonun kendisi DEGIL, tabloya ISARETCI (ROM once
 * `ldr r0,[r0,#0]` ile icerigini okuyor). Indis 400'u asarsa 0 doner;
 * sinir dahil (`bhi`).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/get_record16.c
 */

#include "gba_types.h"

#define RECORD_MAX 400

typedef struct Record {
    u8 pad00[16];               /* adim 16; ic yapisi BILINMIYOR */
} Record;

extern Record *gRam0201AA98;

/* 0x0800DB60 */
Record *GetRecord16(u32 index)
{
    if (index > RECORD_MAX)
        return 0;
    return &gRam0201AA98[index];
}
