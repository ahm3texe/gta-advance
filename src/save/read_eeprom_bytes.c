/* Aligned EEPROM read — 0x0800091C-0x080009EB
 *
 * Reads 8-byte blocks from the Nintendo EEPROM routine and transfers them into
 * the destination buffer with the byte order reversed.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/save/read_eeprom_bytes.c
 */

#include "gba_io.h"

#define DMA_ENABLE   0x80000000
#define EEPROM_BLOCK 8

extern void FUN_0806bdfc(u16 block, void *dest);

#define COPY_EEPROM_BYTE(index) \
    if (size < 0)               \
        continue;               \
    *dest++ = buffer[index];    \
    size--

/* 0x0800091C */
u32 ReadEepromBytes(u32 block, s32 size, u8 *dest)
{
    u8 buffer[EEPROM_BLOCK];
    s32 blocks;
    s32 i;

    blocks = size / EEPROM_BLOCK;

    while (REG_DMA3.control & DMA_ENABLE)
        ;

    REG_IME = 0;

    for (i = 0; i < blocks; i++) {
        FUN_0806bdfc((u16)(block + i), buffer);

        /* The EEPROM word arrives big-endian; the bytes are transferred in
         * reverse order. In the ROM these eight copies are written out
         * explicitly: the form produced with compiler flags (-funroll-loops)
         * does not match the ROM's. */
        COPY_EEPROM_BYTE(7);
        COPY_EEPROM_BYTE(6);
        COPY_EEPROM_BYTE(5);
        COPY_EEPROM_BYTE(4);
        COPY_EEPROM_BYTE(3);
        COPY_EEPROM_BYTE(2);
        COPY_EEPROM_BYTE(1);
        COPY_EEPROM_BYTE(0);
    }

    REG_IME = 1;
    return 1;
}
