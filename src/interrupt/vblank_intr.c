/* VBlank kesme isleyicisi — 0x08000220-0x0800038C
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/interrupt/vblank_intr.c
 *
 * VBlank'a girildiginde iki kare sayaci artirilir, sonra yuvalama sayaci
 * (gEepromAvailable) bir artirilir. Sayac 1 degilse isleyici zaten
 * calisiyordur; is yapilmadan cikilir. Asil is VCOUNT'a bakarak yapilir:
 * VBlank penceresinin ne kadari kaldiysa aktarim adimlarinin tamami ya da
 * hicbiri calistirilir. gVBlankState bu karari kareler arasinda tasiyan
 * kucuk durum makinesidir.
 */

#include "gba_io.h"

/* IWRAM adresi sabit cast olarak yazilir, extern sembol olarak degil:
 * ROM 0x03000000'i kaydirmayla uretiyor (movs #0xc0 / lsls #18), sembol
 * olsaydi literal havuzdan okunurdu. (docs/COMPILER.md, kural 1'in istisnasi) */
#define gFrameDelay (*(u32 *)0x03000000)

/* VBlank 160. tarama satirinda baslar; VCOUNT - 160 gecen satir sayisidir. */
#define VBLANK_FIRST_LINE 160
/* Kisa butce: durum makinesi zaten hazir oldugunda aktarima izin verilen
 * gecikme. Uzun butce: ilk kez baslarken kabul edilen gecikme. */
#define BUDGET_SHORT 10
#define BUDGET_LONG  19

#define FRAME_DELAY_MAX 5

/* gVBlankState degerleri */
#define VBLANK_IDLE  0   /* bir sonraki VBlank'ta aktarim denenecek */
#define VBLANK_BUSY  1   /* aktarim bu karede yapildi                */
#define VBLANK_READY 2   /* pencere kacti, gelecek karede dogrudan aktar */

extern u32 gFrameCounterLate;
extern u32 gIwramFrameCounter;
/* Ayni sozcuk hem EEPROM bulundu bayragi hem kesme yuvalama sayaci olarak
 * kullaniliyor (data/ram_map.csv 0x02000EB8). */
extern u32 gEepromAvailable;
extern u16 gVBlankEnabled;
/* volatile SART: kaldirilinca agbcc uc karsilastirmanin tekini yukleyip
 * degeri callee-saved register'da tutuyor; ROM her dalda yeniden okuyor
 * (0x8000270, 0x80002E0, 0x8000358). volatile burada semantik degil
 * siralama/yeniden-yukleme dugmesi — docs/COMPILER.md kural 4. */
extern volatile u8 gVBlankState;
extern u32 gAsyncState;
extern u8  gGameState[16];

/* VBlank sirasinda calisan alt sistemler; henuz adlandirilmadi. */
extern void FUN_08033264(void);
extern void FUN_08011ec4(void);
extern void FUN_0806686c(void);
extern void FlushSpriteList(void);
extern void FUN_080133a8(void);
extern void FUN_080130f4(void);
extern void FlushPaletteQueue(void);
extern void FUN_080101d8(void);
extern void FUN_080108f4(void);
extern void NoOpVBlankFinalize(void);

/* 0x08000220 */
void VBlankIntr(void)
{
    u32 depth;
    u32 counter;
    u8 phase;

    gFrameCounterLate++;
    gIwramFrameCounter++;

    /* Yuvalanmis girisler isi tekrarlamaz, yalnizca sayaci tasir. */
    depth = ++gEepromAvailable;
    if (depth == 1) {
        FUN_08033264();
        FUN_08011ec4();
        if (gVBlankEnabled != 0)
            FUN_0806686c();

        if ((u16)(REG_VCOUNT - VBLANK_FIRST_LINE) <= BUDGET_SHORT) {
            if (gVBlankState == VBLANK_READY) {
                /* Gecen karede pencere kacirilmisti: dogrudan aktar. */
                FlushSpriteList();
                FUN_080133a8();
                FUN_080130f4();
                FlushPaletteQueue();
                if (gAsyncState == 0)
                    FUN_080101d8();

                gFrameDelay = counter = gIwramFrameCounter;
                if (counter > FRAME_DELAY_MAX)
                    gFrameDelay = FRAME_DELAY_MAX;
                phase = gGameState[12] - 1;
                if (phase <= 1)
                    gFrameDelay = FRAME_DELAY_MAX;

                gIwramFrameCounter = 0;
                gVBlankState = VBLANK_BUSY;
            } else if (gVBlankState == VBLANK_IDLE) {
                if (gAsyncState == 0)
                    FUN_080108f4();

                /* Ust adim zaman yemis olabilir; VCOUNT yeniden okunur.
                 * Kosul ROM'daki gibi ters yazili: agbcc boylece "then"
                 * dalini once yerlestiriyor (cmp #19 / bls). */
                if ((u16)(REG_VCOUNT - VBLANK_FIRST_LINE) > BUDGET_LONG) {
                    gVBlankState = VBLANK_READY;
                } else {
                    FlushSpriteList();
                    FUN_080133a8();
                    FUN_080130f4();
                    FlushPaletteQueue();
                    if (gAsyncState == 0)
                        FUN_080101d8();

                    gFrameDelay = counter = gIwramFrameCounter;
                    if (counter > FRAME_DELAY_MAX)
                        gFrameDelay = FRAME_DELAY_MAX;
                    phase = gGameState[12] - 1;
                    if (phase <= 1)
                        gFrameDelay = FRAME_DELAY_MAX;

                    gIwramFrameCounter = 0;
                    gVBlankState = VBLANK_BUSY;
                }
            } else if (gVBlankState != VBLANK_BUSY) {
                gVBlankState = VBLANK_IDLE;
            }
        }

        NoOpVBlankFinalize();
    }

    gEepromAvailable--;
    gBiosIrqFlags |= 1;
}

/* Notlar (denenip olculenler):
 *  - Aktarim blogu iki kez acik yazilmistir. Ortak bir yardimci fonksiyona
 *    alinirsa agbcc -O2 onu inline etmiyor ve iki `bl` uretiyor; ROM'da kod
 *    iki kez kopyalanmis durumda (docs/COMPILER.md kural 14 ile ayni mantik).
 *  - gVBlankState volatile olmadan: 356 byte, 168 byte farkli. Derleyici
 *    ilk `ldrb`in sonucunu r4'te tutup ikinci ve ucuncu karsilastirmada
 *    yeniden kullaniyor, adres ise r7'ye gidiyor; ROM tam tersini yapiyor
 *    (adres r4'te, deger her dalda yeniden okunuyor).
 *  - Ikinci VCOUNT kosulu duz yazildiginda (`<= BUDGET_LONG`) agbcc
 *    `bhi else` uretiyor; ROM `bls then` kullaniyor, yani kaynakta kosul
 *    ters yazilmis. Bu ayni zamanda ikinci literal havuzunun yerini de
 *    degistiriyor (ROM'da havuz `movs #2 / b` ile aktarim blogu arasinda).
 *  - gFrameCounterLate / gEepromAvailable / gVBlankEnabled / gAsyncState /
 *    gGameState / gBiosIrqFlags extern sembol olarak birakildi: ROM hepsini
 *    literal havuzdan okuyor. gFrameDelay tam tersi — kaydirmayla uretiliyor.
 */
