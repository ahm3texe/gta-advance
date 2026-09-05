/* Butun giris tablolarini DMA ile sifirlama — 0x08023CC4-0x08023DEF
 *
 * Dokuz ayri giris tablosunu (hepsi 148 baytlik girislerden olusuyor)
 * DMA3 ile sifirliyor, sonunda iki kucuk durum degiskenini de sifirliyor.
 * Her blok ayni kalibi tekrarliyor: IME'yi kaydet, kesmeleri kapat,
 * yigindaki sifir sozcugunu kaynak yapip sabit adrese DMA fill (0x85000000
 * = etkin + 32 bit + kaynak sabit), kontrol yazmacini olu okuyup DMA'yi
 * bekle, IME'yi geri yukle. Kalip src/text/clear_text_area.c ve
 * src/world/dma_flush.c ile ayni.
 *
 * Sozcuk sayilari 148'in katlarina denk geliyor, yani her hedef bir giris
 * tablosu:  555=15 giris, 148=4, 37=1, 740=20, 37=1, 148=4, 185=5, 185=5,
 * 37=1.  Ilki gEntriesA (0x02023A00, 15 giris x 148 = 2220 bayt) — yani
 * bu fonksiyon gEntriesA ailesinin toplu ilklendiricisi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_b2.c
 */

#include "gba_io.h"

#define ENTRY_STRIDE   148
#define ENTRIES_A_LEN  15

/* DMA3 kontrol: etkin | 32 bit | kaynak sabit; alt 16 bit sozcuk sayisi. */
#define DMA_FILL_32    0x85000000

#define WORDS_A        555      /* 15 giris */
#define WORDS_B        148      /*  4 giris */
#define WORDS_C         37      /*  1 giris */
#define WORDS_D        740      /* 20 giris */
#define WORDS_E        185      /*  5 giris */

/* Bu adreslerin data/ram_map.csv'de karsiligi YOK; ekleme yetkim olmadigi
 * icin sabit cast olarak yazildilar (adres tek basina kullaniliyor, taban
 * + ofset katlanmasi soz konusu degil — kural 1'in gerekcesi burada yok).
 * Karsiligi olanlar yorumda gosterildi. */
#define TABLE_02025280 ((void *)0x02025280)     /* gRam02025280 */
#define TABLE_020245B0 ((void *)0x020245B0)
#define TABLE_020246F0 ((void *)0x020246F0)     /* gRam020246F0 */
#define TABLE_020242B0 ((void *)0x020242B0)
#define TABLE_02024350 ((void *)0x02024350)
#define TABLE_020254D0 ((void *)0x020254D0)
#define TABLE_02023710 ((void *)0x02023710)
#define TABLE_02024650 ((void *)0x02024650)     /* gRam02024650 */

#define STATE_020245A0 (*(u8 *)0x020245A0)
#define STATE_02024344 (*(u16 *)0x02024344)

/* Bu ceviri biriminin gEntriesA gorunumu: icerigi kullanilmiyor, yalnizca
 * DMA hedefi ve boyutu gerekiyor (bkz. src/world/entries_a1.c). */
typedef struct Entry {
    u8 pad00[ENTRY_STRIDE];
} Entry;

extern Entry gEntriesA[ENTRIES_A_LEN];

/* 0x08023CC4 */
void ClearEntryTables(void)
{
    volatile u32 fill;
    u16 ime;
    volatile DmaChannel *dma;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma = (volatile DmaChannel *)REG_DMA3_ADDR;
    dma->src = &fill;
    dma->dst = gEntriesA;
    dma->control = DMA_FILL_32 | WORDS_A;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_02025280;
    dma->control = DMA_FILL_32 | WORDS_B;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_020245B0;
    dma->control = DMA_FILL_32 | WORDS_C;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_020246F0;
    dma->control = DMA_FILL_32 | WORDS_D;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_020242B0;
    dma->control = DMA_FILL_32 | WORDS_C;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_02024350;
    dma->control = DMA_FILL_32 | WORDS_B;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_020254D0;
    dma->control = DMA_FILL_32 | WORDS_E;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_02023710;
    dma->control = DMA_FILL_32 | WORDS_E;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_02024650;
    dma->control = DMA_FILL_32 | WORDS_C;
    dma->control;
    REG_IME = ime;

    STATE_020245A0 = 0;
    STATE_02024344 = 0;
}
