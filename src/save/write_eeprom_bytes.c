/* Hizali EEPROM yazimi — 0x080009EC-0x08000B00
 *
 * Kaynak tamponu 8 byte'lik EEPROM bloklarina ters byte sirasiyla paketler,
 * her blogu yazar ve geri okuyarak dogrular. Dogrulama basarisiz olursa blok
 * yeniden yazilir; deneme sayaci butun cagri boyunca ortaktir, blok basina
 * sifirlanmaz (ROM'da 'mov sl, r0' dis dongunun disinda).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/save/write_eeprom_bytes.c
 */

#include "gba_io.h"

/* IO register'lari sabit cast: ROM bunlari literal havuzdan tek taban olarak
 * okuyup ofsetliyor (ldr r0, [r2, #8]), extern sembol degil. */
#define DMA_ENABLE   0x80000000
#define EEPROM_BLOCK 8

/* Bir blok icin izin verilen yeniden yazma denemesi. ROM'daki karsilastirma
 * isaretli: cmp #19 / ble. */
#define MAX_RETRIES  20

/* Nintendo EEPROM rutinleri; data/functions.csv'de henuz adlandirilmadi.
 * FUN_0806beac bir sozcuk programlar, FUN_0806c020 geri okuyup karsilastirir
 * ve sifirdan farkli bir u16 ile hatayi bildirir. */
extern void FUN_0806beac(u16 block, const void *buffer);
extern u16  FUN_0806c020(u16 block, const void *buffer);

/* EEPROM sozcugu big-endian yazilir: kaynagin ilk byte'i blogun son byte'ina
 * gider. Kaynak bitince (size < 0) blogun kalani dokunulmaz -- bu yuzden
 * paketleme do/while(0) icine sarilip 'break' ile terk edilir; 'continue'
 * olsaydi programlama adimi da atlanirdi. Sekiz kopya acik yazilir, dongu
 * hali farkli kod uretiyor (COMPILER.md kural 14). */
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

        /* 'block + i' bir yerel degiskene ALINMAZ. Yerel degiskenle
         * (u32 word = block + i) uretilen kod baska turlu her yerde ayni,
         * ama register dagitimi kayiyor: derleyici degiskeni r8'e koyup
         * dongu sayacini r6'da tutuyor. ROM ise 'block + i' CSE gecicisini
         * callee-saved r4'te tutup sayaci sp+16'ya tasiyor (bu yuzden
         * 'sub sp, #20', #16 degil). Ifade uc yerde de acik yazilinca
         * agbcc ROM'un dagitimini uretiyor. */
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
