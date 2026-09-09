/* Unaligned EEPROM byte range read — 0x08000DDC-0x08000F1C
 *
 * The EEPROM hardware can only be read in 8-byte blocks. To allow a read of
 * arbitrary length from an arbitrary byte offset, this routine splits the
 * offset into a "block index" plus a "number of bytes to skip inside the
 * block". Each block is read into a temporary buffer on the stack, and only
 * the requested bytes are transferred from the buffer to the destination.
 * Since the EEPROM word arrives big-endian, copying proceeds from 7 down to 0.
 *
 * In the first block the leading `skip` bytes are skipped; in the last block
 * the remaining bytes are dropped once `length` is exhausted; the blocks in
 * between are copied in full.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/save/read_eeprom_range.c
 */

#include "gba_io.h"

#define DMA_ENABLE 0x80000000

#define EEPROM_BLOCK       8    /* the EEPROM access unit (bytes)        */
#define EEPROM_BLOCK_MASK  7    /* byte offset inside a block            */
#define EEPROM_BLOCK_SHIFT 3    /* byte offset -> block index            */
#define EEPROM_DEVICE_TYPE 4    /* type code passed to the ID routine    */

/* 0x02000EB8: while nonzero, the EEPROM is identified and access is live. */
extern u32 gEepromAvailable;

/* Entry to and exit from the save I/O region (WAITCNT and similar setup);
 * their names are not yet resolved in data/functions.csv. */
extern void StopAudioDmaOnCartFlag(void);
extern void FUN_08033b74(void);

/* EEPROM identification and single-block read; both report an error with a
 * nonzero u16. */
extern u16 FUN_0806bd34(u32 deviceType);
extern u16 FUN_0806bdfc(u16 block, void *dest);

/* The eight copies are written out explicitly (rule 14): a byte is
 * transferred once the skip counter is exhausted and while room remains in the
 * destination. `skip` must be signed -- the ROM emits `ble` (signed); if it
 * were unsigned, `bls` would come out (rule 9). */
#define COPY_EEPROM_BYTE(index)  \
    if (skip > 0)                \
        skip--;                  \
    else if (length != 0) {      \
        *dest++ = buffer[index]; \
        length--;                \
    }

/* 0x08000DDC */
u32 ReadEepromRange(u32 offset, u8 *dest, s32 length)
{
    u8 buffer[EEPROM_BLOCK];
    s32 skip;
    s32 i;

    StopAudioDmaOnCartFlag();

    /* The offset is converted to a block index in place. With a separate
     * `block` variable agbcc first takes it into r4 and copies it to r8; the
     * ROM keeps the offset in r8 from the start (the inverse direction of
     * rule 11). */
    skip = offset & EEPROM_BLOCK_MASK;
    offset >>= EEPROM_BLOCK_SHIFT;

    REG_IME = 0;
    while (REG_DMA3.control & DMA_ENABLE)
        ;

    gEepromAvailable = 1;

    if (FUN_0806bd34(EEPROM_DEVICE_TYPE) == 0) {
        i = 0;
        /* A hand-written loop rotation: the ROM performs the exit test once
         * up front and then enters the body, with the back branch at the end
         * of the body. A `for` or `while` form instead jumps with a `b` to the
         * test at the bottom and moves the read call to the end of the
         * loop. */
        if (length != 0) {
            do {
                /* A read error ends the whole operation; `break` is not
                 * enough, because the compiler then changes the block
                 * ordering. */
                if (FUN_0806bdfc((u16)(offset + i), buffer) != 0)
                    goto finish;

                COPY_EEPROM_BYTE(7);
                COPY_EEPROM_BYTE(6);
                COPY_EEPROM_BYTE(5);
                COPY_EEPROM_BYTE(4);
                COPY_EEPROM_BYTE(3);
                COPY_EEPROM_BYTE(2);
                COPY_EEPROM_BYTE(1);
                COPY_EEPROM_BYTE(0);
                i++;
            } while (length != 0);
        }
    }

finish:
    gEepromAvailable = 0;
    REG_IME = 1;
    FUN_08033b74();
    return 1;
}
