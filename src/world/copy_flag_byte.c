/* gSaveBuffer byte-copy pair — 0x080320C8-0x080320DF
 *
 * The ROM accesses both fields with a single base load:
 *     ldrb r0, [r1, #8] / strb r0, [r1, #9]
 *
 * MEANING UNVERIFIED: the fields' purpose is unknown, so the name describes
 * the operation (copy 8 to 9), not its purpose. gSaveBuffer is the save
 * manager's working buffer (data/ram_map.csv).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/copy_flag_byte.c
 */

#include "gba_types.h"

typedef struct SaveBuffer {
    u8 pad00[8];
    u8 byte8;                   /* +0x08 */
    u8 byte9;                   /* +0x09 */
    u8 pad0A[92];
    u16 counter66;              /* +0x66 */
    u8 pad68[14];
    u16 counter76;              /* +0x76 */
} SaveBuffer;

extern SaveBuffer gSaveBuffer;

/* 0x080320C8 */
void SaveBufferCopy8To9(void)
{
    gSaveBuffer.byte9 = gSaveBuffer.byte8;
}

/* 0x080320D4 */
void SaveBufferCopy9To8(void)
{
    gSaveBuffer.byte8 = gSaveBuffer.byte9;
}
