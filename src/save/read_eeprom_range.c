/* Hizalanmamis EEPROM byte araligi okumasi — 0x08000DDC-0x08000F1C
 *
 * EEPROM donanimi yalnizca 8 byte'lik bloklar halinde okunabilir. Bu rutin
 * rastgele bir byte ofsetinden rastgele uzunlukta okuma yapabilmek icin
 * ofseti blok indeksi + blok ici atlama sayisina ayirir, her blogu yigin
 * uzerindeki gecici tampona okur ve tampondan yalnizca istenen byte'lari
 * hedefe kopyalar. EEPROM sozcugu big-endian geldigi icin kopyalama
 * 7'den 0'a dogru ilerler.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/save/read_eeprom_range.c
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

#define EEPROM_BLOCK      8         /* EEPROM erisim birimi (byte)         */
#define EEPROM_BLOCK_MASK 7         /* blok ici byte ofseti maskesi        */
#define EEPROM_BLOCK_SHIFT 3        /* byte ofsetinden blok indeksine      */
#define EEPROM_DEVICE_TYPE 4        /* tanimlama rutinine verilen tip kodu */

/* 0x02000EB8: sifirdan farkliyken EEPROM tanimlanmis ve mesgul demektir. */
extern u32 gEepromAvailable;

/* Kayit G/C bolgesine giris/cikis; adlari henuz kesinlesmedi. */
extern void FUN_080337a8(void);
extern void FUN_08033b74(void);

/* EEPROM tanimlama ve tek blok okuma; ikisi de sifir olmayan u16 ile hata
 * bildirir. */
extern u16 FUN_0806bd34(u32 deviceType);
extern u16 FUN_0806bdfc(u16 block, void *dest);

/* Sekiz kopya acik yazilir (kural 14): tampondaki byte, atlama sayaci
 * bittikten sonra ve hedefte yer kaldigi surece aktarilir. */
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
    u32 block;
    s32 skip;
    s32 i;

    FUN_080337a8();

    skip = offset & EEPROM_BLOCK_MASK;
    block = offset >> EEPROM_BLOCK_SHIFT;

    REG_IME = 0;
    while (REG_DMA3.control & DMA_ENABLE)
        ;

    gEepromAvailable = 1;

    if (FUN_0806bd34(EEPROM_DEVICE_TYPE) == 0) {
        for (i = 0; length != 0; i++) {
            if (FUN_0806bdfc((u16)(block + i), buffer) != 0)
                break;

            COPY_EEPROM_BYTE(7);
            COPY_EEPROM_BYTE(6);
            COPY_EEPROM_BYTE(5);
            COPY_EEPROM_BYTE(4);
            COPY_EEPROM_BYTE(3);
            COPY_EEPROM_BYTE(2);
            COPY_EEPROM_BYTE(1);
            COPY_EEPROM_BYTE(0);
        }
    }

    gEepromAvailable = 0;
    REG_IME = 1;
    FUN_08033b74();
    return 1;
}
