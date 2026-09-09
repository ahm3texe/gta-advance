/* Write an unaligned EEPROM byte range — 0x08000F1C-0x08001094
 *
 * The write counterpart of ReadEepromRange (0x08000DDC). Because EEPROM can
 * only be programmed in 8-byte blocks, the routine splits the byte offset
 * into a "block index" (offset >> 3) and a "number of bytes to skip inside
 * the block" (offset & 7). Each block is first assembled in an 8-byte buffer
 * on the stack, then programmed in one go. Since the EEPROM word arrives
 * big-endian, the buffer is filled from 7 down to 0.
 *
 * Two things differ from the read side:
 *   - The number of blocks to write is computed up front and the range is
 *     validated against the 64-block (512-byte) limit; a request that
 *     overflows fails without writing anything.
 *   - Programming is attempted at most 10 times per block (retry <= 9); if
 *     the tenth attempt also fails, the whole operation is considered failed.
 *
 * Returns: 1 on a successful write, 0 on an identification, range or
 * programming error.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/save/write_eeprom_range.c
 */

#include "gba_io.h"

/* The inverse direction of rule 1: DMA3 is written as a constant cast, but
 * as a struct member rather than a standalone `*(volatile u32 *)0x040000DC`.
 * In the standalone form agbcc folds base+offset into a single literal
 * (0x040000DC / [r2,#0]); the ROM instead keeps the base in a register and
 * reaches it by offset (0x040000D4 / [r2,#8]). */
#define DMA_ENABLE 0x80000000

#define EEPROM_BLOCK       8    /* EEPROM access unit (bytes)           */
#define EEPROM_BLOCK_MASK  7    /* byte offset inside a block           */
#define EEPROM_BLOCK_SHIFT 3    /* byte offset -> block index           */
#define EEPROM_DEVICE_TYPE 4    /* type code passed to the ID routine   */
#define EEPROM_BLOCKS      64   /* 4 Kbit EEPROM = 64 x 8 bytes         */
#define EEPROM_MAX_RETRY   9    /* give up after the tenth attempt      */

/* 0x02000EB8: while nonzero, the EEPROM is identified and access is live. */
extern u32 gEepromAvailable;

/* Entry to and exit from the save I/O region; their names are not yet
 * resolved in data/functions.csv. */
extern void StopAudioDmaOnCartFlag(void);
extern void FUN_08033b74(void);

/* EEPROM identification and single-block programming; both report an error
 * with a nonzero u16 (the ROM tests the return value with `lsls #16`). */
extern u16 FUN_0806bd34(u32 deviceType);
extern u16 FUN_0806c078(u16 block, const void *data);

/* The eight copies are written out explicitly (rule 14): a byte is taken
 * into the buffer once the skip counter is exhausted and while bytes remain
 * in the source. `skip` must be signed -- the ROM emits `ble` (signed); if it
 * were unsigned, `bls` would come out (rule 9). */
#define COPY_EEPROM_BYTE(index)  \
    if (skip > 0)                \
        skip--;                  \
    else if (length != 0) {      \
        buffer[index] = *src++;  \
        length--;                \
    }

/* 0x08000F1C */
u32 WriteEepromRange(u32 offset, const u8 *src, u32 length)
{
    u8 buffer[EEPROM_BLOCK];
    s32 blockCount;
    s32 skip;
    s32 i;
    s32 retry;
    u16 err;
    u32 result;

    StopAudioDmaOnCartFlag();

    /* `length` must be unsigned: the ROM does (length-1)/8 with `lsrs`; if
     * it were signed, `asrs` would come out (rule 13). The block count, by
     * contrast, is signed, because the loop comparison is `bge`/`blt`
     * (rule 9). */
    blockCount = ((length - 1) >> EEPROM_BLOCK_SHIFT) + 1;
    skip = offset & EEPROM_BLOCK_MASK;
    /* The offset is converted to a block index in place; a separate `block`
     * variable produces an extra register copy (the same holds in
     * ReadEepromRange). */
    offset >>= EEPROM_BLOCK_SHIFT;

    REG_IME = 0;
    while (REG_DMA3.control & DMA_ENABLE)
        ;

    gEepromAvailable = 1;
    result = 1;

    if (FUN_0806bd34(EEPROM_DEVICE_TYPE) != 0) {
        result = 0;
    } else if (offset + blockCount > EEPROM_BLOCKS) {
        result = 0;
    } else {
        for (i = 0; i < blockCount; i++) {
            COPY_EEPROM_BYTE(7);
            COPY_EEPROM_BYTE(6);
            COPY_EEPROM_BYTE(5);
            COPY_EEPROM_BYTE(4);
            COPY_EEPROM_BYTE(3);
            COPY_EEPROM_BYTE(2);
            COPY_EEPROM_BYTE(1);
            COPY_EEPROM_BYTE(0);

            /* The counter is incremented right after the call, before the
             * error test: the ROM puts `adds r6,#1` ahead of `cmp r0,#0`, so
             * the increment does not depend on the error branch. Measured:
             * writing the same logic as
             * `while ((err = ...) != 0) { if (++retry > 9) ... }` produces
             * 372 bytes / 167 differences -- the increment moves inside the
             * error branch and the block ordering changes from the start. */
            retry = 0;
            do {
                /* The block address is rewritten as `(u16)(offset + i)` on
                 * every iteration. Incrementing a separate `u16 block`
                 * variable (block++) produces `add / lsl #16 / lsr #16` in
                 * every loop; the ROM instead keeps the value shifted left by
                 * 16 (r8 += 0x10000) and extracts it at the use site with
                 * `lsrs r0,r2,#16`. That form comes out of agbcc's loop
                 * strength reduction, and only when the truncation is written
                 * at the call site. */
                err = FUN_0806c078((u16)(offset + i), buffer);
                retry++;
            } while (err != 0 && retry <= EEPROM_MAX_RETRY);

            if (err != 0) {
                result = 0;
                break;
            }
        }
    }

    gEepromAvailable = 0;
    REG_IME = 1;
    FUN_08033b74();
    return result;
}
