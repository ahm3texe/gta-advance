/* Aligned EEPROM write — 0x080009EC-0x08000B00
 *
 * Packs the source buffer into 8-byte EEPROM blocks in reverse byte order,
 * writes each block and validates it by reading it back. If validation fails
 * the block is rewritten; the attempt counter is shared across the whole call
 * and is not reset per block (in the ROM 'mov sl, r0' sits outside the outer
 * loop).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/save/write_eeprom_bytes.c
 */

#include "gba_io.h"

/* The IO registers are constant casts: the ROM reads them from the literal
 * pool as a single base and offsets from it (ldr r0, [r2, #8]), not as extern
 * symbols. */
#define DMA_ENABLE   0x80000000
#define EEPROM_BLOCK 8

/* Rewrite attempts allowed for one block. The comparison in the ROM is
 * signed: cmp #19 / ble. */
#define MAX_RETRIES  20

/* Nintendo EEPROM routines; not yet named in data/functions.csv.
 * FUN_0806beac programs one word, FUN_0806c020 reads it back, compares it and
 * reports an error with a nonzero u16. */
extern void FUN_0806beac(u16 block, const void *buffer);
extern u16  FUN_0806c020(u16 block, const void *buffer);

/* The EEPROM word is written big-endian: the source's first byte goes to the
 * block's last byte. Once the source runs out (size < 0) the rest of the block
 * is left untouched -- that is why the packing is wrapped in a do/while(0) and
 * left with 'break'; with 'continue' the programming step would be skipped as
 * well. The eight copies are written out explicitly; a loop form produces
 * different code (COMPILER.md rule 14). */
#define COPY_EEPROM_BYTE(index) \
    if (size < 0)               \
        break;                  \
    buffer[index] = *src++;     \
    size--

/* 0x080009EC */
u32 WriteEepromBytes(u32 block, s32 size, const u8 *src)
{
    u8 buffer[EEPROM_BLOCK];
    s32 blocks;
    s32 retries;
    s32 i;

    blocks = size / EEPROM_BLOCK;

    while (REG_DMA3.control & DMA_ENABLE)
        ;

    REG_IME = 0;
    retries = 0;

    for (i = 0; i < blocks; i++) {
        do {
            COPY_EEPROM_BYTE(7);
            COPY_EEPROM_BYTE(6);
            COPY_EEPROM_BYTE(5);
            COPY_EEPROM_BYTE(4);
            COPY_EEPROM_BYTE(3);
            COPY_EEPROM_BYTE(2);
            COPY_EEPROM_BYTE(1);
            COPY_EEPROM_BYTE(0);
        } while (0);

        /* 'block + i' is NOT taken into a local variable. With a local
         * (u32 word = block + i) the generated code is otherwise identical
         * everywhere, but the register allocation shifts: the compiler puts
         * the variable in r8 and keeps the loop counter in r6. The ROM instead
         * keeps the 'block + i' CSE temporary in callee-saved r4 and moves the
         * counter to sp+16 (which is why it is 'sub sp, #20', not #16).
         * Writing the expression out at all three sites makes agbcc produce
         * the ROM's allocation. */
        FUN_0806beac((u16)(block + i), buffer);

        while (FUN_0806c020((u16)(block + i), buffer) != 0
               && retries < MAX_RETRIES) {
            FUN_0806beac((u16)(block + i), buffer);
            retries++;
        }
    }

    REG_IME = 1;
    return 1;
}
