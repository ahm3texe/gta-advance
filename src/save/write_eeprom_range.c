/* Hizalanmamis EEPROM byte araligi yazma — 0x08000F1C-0x08001094
 *
 * ReadEepromRange'in (0x08000DDC) yazma esi. EEPROM yalnizca 8 byte'lik
 * bloklar halinde programlanabildigi icin rutin, byte ofsetini "blok
 * indeksi" (offset >> 3) ve "blok ici atlanacak byte sayisi" (offset & 7)
 * olarak ikiye ayirir. Her blok once yigin uzerindeki 8 byte'lik tampona
 * kurulur, sonra tek seferde programlanir. EEPROM sozcugu big-endian
 * geldigi icin tampon 7'den 0'a dogru doldurulur.
 *
 * Okumadan farkli iki nokta var:
 *   - Yazilacak blok sayisi bastan hesaplanir ve aralik 64 blok (512 byte)
 *     sinirina karsi dogrulanir; tasan istek hicbir sey yazmadan basarisiz
 *     doner.
 *   - Her blok icin programlama en fazla 10 kez denenir (retry <= 9);
 *     onuncu deneme de hata verirse tum islem basarisiz sayilir.
 *
 * Donus: basarili yazmada 1, tanimlama/aralik/programlama hatasinda 0.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/save/write_eeprom_range.c
 */

#include "gba_io.h"

/* Kural 1'in tersi yonu: DMA3 sabit cast olarak yazilir, ama tekil bir
 * `*(volatile u32 *)0x040000DC` degil struct uyesi olarak. Tekil biciminde
 * agbcc taban+ofseti tek literale katliyor (0x040000DC / [r2,#0]); ROM ise
 * tabani register'da tutup ofsetle eriyor (0x040000D4 / [r2,#8]). */
#define DMA_ENABLE 0x80000000

#define EEPROM_BLOCK       8    /* EEPROM erisim birimi (byte)          */
#define EEPROM_BLOCK_MASK  7    /* blok ici byte ofseti                 */
#define EEPROM_BLOCK_SHIFT 3    /* byte ofseti -> blok indeksi          */
#define EEPROM_DEVICE_TYPE 4    /* tanimlama rutinine verilen tip kodu  */
#define EEPROM_BLOCKS      64   /* 4 Kbit EEPROM = 64 x 8 byte          */
#define EEPROM_MAX_RETRY   9    /* onuncu denemeden sonra vazgecilir    */

/* 0x02000EB8: sifirdan farkliyken EEPROM tanimli ve erisim suruyor. */
extern u32 gEepromAvailable;

/* Kayit G/C bolgesine giris/cikis; adlari data/functions.csv'de henuz
 * cozulmedi. */
extern void FUN_080337a8(void);
extern void FUN_08033b74(void);

/* EEPROM tanimlama ve tek blok programlama; ikisi de sifirdan farkli bir
 * u16 ile hata bildirir (ROM donus degerini `lsls #16` ile sinayor). */
extern u16 FUN_0806bd34(u32 deviceType);
extern u16 FUN_0806c078(u16 block, const void *data);

/* Sekiz kopya acik yazilir (kural 14): bir byte, atlama sayaci bittikten
 * sonra ve kaynakta byte kaldigi surece tampona alinir. `skip` isaretli
 * olmali — ROM `ble` (isaretli) uretiyor, isaretsiz olsaydi `bls` cikardi
 * (kural 9). */
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

    FUN_080337a8();

    /* `length` isaretsiz olmali: ROM (length-1)/8'i `lsrs` ile yapiyor,
     * isaretli olsaydi `asrs` cikardi (kural 13). Blok sayisi ise isaretli,
     * cunku dongu karsilastirmasi `bge`/`blt` (kural 9). */
    blockCount = ((length - 1) >> EEPROM_BLOCK_SHIFT) + 1;
    skip = offset & EEPROM_BLOCK_MASK;
    /* Ofset yerinde blok indeksine cevrilir; ayri bir `block` degiskeni
     * fazladan bir register kopyasi uretiyor (ReadEepromRange'te de oyle). */
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

            /* Sayac cagridan hemen sonra, hata sinamasindan once artiyor:
             * ROM `adds r6,#1`i `cmp r0,#0`in onune koyuyor, yani artirma
             * hata daline bagli degil. Olculdu: ayni mantigi
             * `while ((err = ...) != 0) { if (++retry > 9) ... }` biciminde
             * yazmak 372 byte / 167 fark uretiyor — artirma hata dalinin
             * icine giriyor ve blok siralamasi bastan degisiyor. */
            retry = 0;
            do {
                /* Blok adresi her turda `(u16)(offset + i)` olarak yeniden
                 * yazilir. Ayri bir `u16 block` degiskenini artirmak
                 * (block++) her dongude `add / lsl #16 / lsr #16` uretiyor;
                 * ROM ise degeri 16 bit kaydirilmis tutup (r8 += 0x10000)
                 * kullanirken `lsrs r0,r2,#16` ile ayikliyor. Bu bicim
                 * agbcc'nin dongu kuvvet indirgemesinden cikiyor ve ancak
                 * kirpma cagri yerinde yazilinca olusuyor. */
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
