/* Kart bayragi kurulunca ses DMA'sini durdurma — 0x080337A8-0x0803381F
 *
 * gCartFlag'in degeri once gRam02027314'e kopyalaniyor; 1 degilse hicbir
 * sey yapilmiyor. 1 ise bayrak sifirlanip DMA1 ve DMA2 (GBA'da ses FIFO
 * kanallari) iki adimda kapatiliyor: once 0xC5FF ile denetim bitleri,
 * sonra 0x7FFF ile etkinlestirme biti temizleniyor; her yazimin ardindan
 * ROM'da bir geri okuma var. Son olarak gRam02027310'daki tampon
 * biraktiriliyor.
 *
 * UC AYRINTI OLCULDU:
 *
 * 1) Kosul YEREL bir u8'e degil, gRam02027314'un KENDISINE bakiyor.
 *    Yerel kullanilirsa ROM'daki `lsls #24 / lsrs #24` sifir genisletmesi
 *    cikmiyor (ldrb zaten genisletilmis oluyor).
 *
 * 2) Denetim yazmaci `vu16 *` TABAN + INDIS ile yazilmali. gba_io.h'daki
 *    `REG_DMA1.control` struct gorunumu her yazimdan once fazladan bir
 *    volatile OKUMA uretiyor (kanal basina iki fazla ldrh). Mutlak
 *    makro (`((vu16 *)0x040000BC)[5]`) ise +10 uzakligini adres sabitine
 *    katlayip havuza 0x040000C6 koyuyor; ROM'da taban 0x040000BC ve
 *    uzaklik 10.
 *
 * 3) IKINCI taban kendi kullanim yerinde atanmali. Ikisi de basta
 *    atanirsa iki havuz yuklemesi de basa toplaniyor; tek degiskene iki
 *    kez atanirsa agbcc ikinciyi birincinin +12'si diye ortak altifadeye
 *    cikariyor (`adds r4,#12`).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/stop_audio_dma.c
 */

#include "gba_types.h"

#define DMA1_BASE       0x040000BC
#define DMA2_BASE       0x040000C8
#define DMA_CNT_H       5           /* +0x0A, u16 adimlarla */
#define DMA_CTRL_MASK   0xC5FF      /* ~0x3A00 */
#define DMA_ENABLE_MASK 0x7FFF      /* ~0x8000 */

extern u8   gCartFlag;
extern u8   gRam02027314;
extern u32  gRam02027310;   /* zero_three_flags.c ile ayni gorunum: SAYI, isaretci degil */
extern u8   gBufferBase02014ED0[];

extern void FUN_0800c804(u32 *dest, u32 value);

/* 0x080337A8 */
void StopAudioDmaOnCartFlag(void)
{
    vu16 *p1;
    vu16 *p2;

    gRam02027314 = gCartFlag;
    if (gRam02027314 != 1)
        return;

    gCartFlag = 0;

    p1 = (vu16 *)DMA1_BASE;
    p1[DMA_CNT_H] = DMA_CTRL_MASK & p1[DMA_CNT_H];
    p1[DMA_CNT_H] = DMA_ENABLE_MASK & p1[DMA_CNT_H];
    p1[DMA_CNT_H];

    p2 = (vu16 *)DMA2_BASE;
    p2[DMA_CNT_H] = DMA_CTRL_MASK & p2[DMA_CNT_H];
    p2[DMA_CNT_H] = DMA_ENABLE_MASK & p2[DMA_CNT_H];
    p2[DMA_CNT_H];

    if (gRam02027310 != 0)
        FUN_0800c804((u32 *)gBufferBase02014ED0, gRam02027310);
    gRam02027310 = 0;
}
