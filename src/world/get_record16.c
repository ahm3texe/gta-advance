/* Get an entry from a 16-byte record table — 0x0800DB60-0x0800DB7B
 *
 * gRam0201AA98 is a POINTER to the table, not the table itself (the ROM
 * first reads its contents with ldr r0,[r0,#0]). Return 0 if the index exceeds
 * 400; the boundary is inclusive (bhi).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/get_record16.c
 */

#include "gba_types.h"

#define RECORD_MAX 400

typedef struct Record {
    u8 pad00[16];               /* stride 16; internal layout UNKNOWN */
} Record;

extern Record *gRam0201AA98;

/* 0x0800DB60 */
Record *GetRecord16(u32 index)
{
    if (index > RECORD_MAX)
        return 0;
    return &gRam0201AA98[index];
}
