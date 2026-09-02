/* Hizali EEPROM okumasi — 0x0800091C-0x080009EB
 *
 * Nintendo EEPROM rutininden 8 byte'lik bloklar okur ve byte sirasini
 * ters cevirerek hedef tampona aktarir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/save/read_eeprom_bytes.c
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef signed int     s32;

typedef struct {
    const void *src;
    void *dst;
    u32 control;
} DmaChannel;

#define REG_DMA3     (*(volatile DmaChannel *)0x040000D4)
#define REG_IME      (*(volatile u16 *)0x04000208)
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

        /* EEPROM sozcugu big-endian gelir; byte'lar ters sirayla aktarilir.
         * ROM'da bu sekiz kopya acik yazilmis: derleyici bayraklariyla
         * (-funroll-loops) uretilen bicim ROM'dakine uymuyor. */
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
