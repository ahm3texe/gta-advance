/* Hizali EEPROM yazimi — 0x080009EC-0x08000B00
 *
 * Kaynak tamponu 8 byte'lik EEPROM bloklarina ters byte sirasiyla paketler,
 * her blogu yazar ve geri okuyarak dogrular. Dogrulama basarisiz olursa blok
 * yeniden yazilir; deneme sayaci butun cagri boyunca ortaktir, blok basina
 * sifirlanmaz.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/save/write_eeprom_bytes.c
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

/* Bir blok icin izin verilen en fazla yeniden yazma denemesi. ROM'daki
 * karsilastirma "sayac <= 19" seklinde isaretli. */
#define MAX_RETRIES  20

/* Nintendo EEPROM rutinleri; henuz adlandirilmadi (data/functions.csv). */
extern void FUN_0806beac(u16 block, const void *buffer);
extern u16  FUN_0806c020(u16 block, const void *buffer);

/* EEPROM sozcugu big-endian yazilir: kaynagin ilk byte'i blogun son
 * byte'ina gider. Kaynak bitince (size < 0) kalan byte'lar dokunulmadan
 * birakilir -- bu yuzden 'break', paketleme do/while(0) icine sarili.
 * Sekiz kopya acik yazilir; dongu hali farkli kod uretiyor (kural 14). */
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
    u32 word;

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

        word = block + i;
        FUN_0806beac((u16)word, buffer);

        while (FUN_0806c020((u16)word, buffer) != 0 && retries < MAX_RETRIES) {
            FUN_0806beac((u16)word, buffer);
            retries++;
        }
    }

    REG_IME = 1;
    return 1;
}
