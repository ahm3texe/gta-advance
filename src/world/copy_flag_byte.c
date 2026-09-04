/* gSaveBuffer bayt kopyalama ciftleri — 0x080320C8-0x080320DF
 *
 * ROM tek taban yuklemesiyle iki alana da erisiyor:
 *     ldrb r0, [r1, #8]  /  strb r0, [r1, #9]
 *
 * ANLAM DOGRULANMADI: alanlarin ne oldugu bilinmiyor, bu yuzden ad
 * davranisi tarif ediyor ("8'i 9'a kopyala"), islevi degil. gSaveBuffer
 * kayit yoneticisinin calisma tamponu (data/ram_map.csv).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/copy_flag_byte.c
 */

#include "gba_types.h"

typedef struct SaveBuffer {
    u8 pad00[8];
    u8 byte8;                   /* +0x08 */
    u8 byte9;                   /* +0x09 */
    u8 pad0A[92];
    u16 counter66;              /* +0x66 */
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
