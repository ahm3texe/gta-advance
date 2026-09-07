/* Hizalanmamis EEPROM byte araligi okumasi — 0x08000DDC-0x08000F1C
 *
 * EEPROM donanimi yalnizca 8 byte'lik bloklar halinde okunabilir. Bu rutin
 * rastgele bir byte ofsetinden rastgele uzunlukta okuma yapabilmek icin
 * ofseti "blok indeksi" + "blok ici atlanacak byte sayisi" olarak ikiye
 * ayirir. Her blok yigin uzerindeki gecici tampona okunur, tampondan
 * yalnizca istenen byte'lar hedefe aktarilir. EEPROM sozcugu big-endian
 * geldigi icin kopyalama 7'den 0'a dogru ilerler.
 *
 * Ilk blokta bastaki `skip` byte atlanir, son blokta `length` bitince
 * kalan byte'lar birakilir; aradaki bloklar tam kopyalanir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/save/read_eeprom_range.c
 */

#include "gba_io.h"

#define DMA_ENABLE 0x80000000

#define EEPROM_BLOCK       8    /* EEPROM erisim birimi (byte)           */
#define EEPROM_BLOCK_MASK  7    /* blok ici byte ofseti                  */
#define EEPROM_BLOCK_SHIFT 3    /* byte ofseti -> blok indeksi           */
#define EEPROM_DEVICE_TYPE 4    /* tanimlama rutinine verilen tip kodu   */

/* 0x02000EB8: sifirdan farkliyken EEPROM tanimli ve erisim surüyor. */
extern u32 gEepromAvailable;

/* Kayit G/C bolgesine giris/cikis (WAITCNT ve benzeri kurulum); adlari
 * data/functions.csv'de henuz cozulmedi. */
extern void StopAudioDmaOnCartFlag(void);
extern void FUN_08033b74(void);

/* EEPROM tanimlama ve tek blok okuma; ikisi de sifirdan farkli bir u16 ile
 * hata bildirir. */
extern u16 FUN_0806bd34(u32 deviceType);
extern u16 FUN_0806bdfc(u16 block, void *dest);

/* Sekiz kopya acik yazilir (kural 14): bir byte, atlama sayaci bittikten
 * sonra ve hedefte yer kaldigi surece aktarilir. `skip` isaretli olmali —
 * ROM `ble` (isaretli) uretiyor, isaretsiz olsaydi `bls` cikardi (kural 9). */
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

    /* Ofset yerinde blok indeksine cevrilir. Ayri bir `block` degiskeni
     * kullanilinca agbcc onu once r4'e alip r8'e kopyaliyor; ROM ofseti
     * bastan r8'de tutuyor (kural 11'in tersi yonu). */
    skip = offset & EEPROM_BLOCK_MASK;
    offset >>= EEPROM_BLOCK_SHIFT;

    REG_IME = 0;
    while (REG_DMA3.control & DMA_ENABLE)
        ;

    gEepromAvailable = 1;

    if (FUN_0806bd34(EEPROM_DEVICE_TYPE) == 0) {
        i = 0;
        /* Elle yazilmis dongu dondurmesi: ROM cikis testini once bir kez
         * yapip govdeye giriyor, geri dal govdenin sonunda. `for` ya da
         * `while` yazimi bunun yerine alttaki teste `b` ile atliyor ve
         * okuma cagrisini dongunun sonuna tasiyor. */
        if (length != 0) {
            do {
                /* Okuma hatasi tum islemi bitirir; `break` yeterli degil,
                 * cunku o zaman derleyici blok siralamasini degistiriyor. */
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
