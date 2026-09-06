/* Baglanti kare adimi — 0x0806660C-0x0806673B
 *
 * Her karede bir kez cagriliyor. Once SIO denetim kelimesi tek 32 bitlik
 * okumayla aliniyor, sonra iletisim blogunun +0x01 durum baytina gore uc
 * durumlu bir makine isletiliyor (durumlar birbirine DUSUYOR, break yok):
 *
 *   durum 0 — Kurulum. Yalnizca bu GBA efendiyse (ID alani, bit 4-5, sifir)
 *             devam ediliyor. SD (bit 3) kurulu ve mesgul (bit 7) degilse,
 *             ayrica SI (bit 2) bos ve gonderim uzunlugu 12 ise donanim
 *             seri kesmeden zamanlayici-3 kesmesine cevriliyor:
 *             IME kapali; IE'de seri (bit 7) kapatilip zamanlayici-3
 *             (bit 6) aciliyor; IME acik; SIOCNT'nin kesme biti (bit 14)
 *             siliniyor; TM3CNT tek 32 bitlik yazimla 0xABFB yeniden
 *             yukleme + kapali denetim aliyor; IF'e 0xC0 yazilip bekleyen
 *             iki kesme onaylaniyor; blogun +0x00 baytina 8 yaziliyor.
 *             Kosul tutsa da tutmasa da durum 1'e geciliyor; yalnizca
 *             SD/mesgul kosulu tutmazsa butun makine atlaniyor.
 *   durum 1 — Bekleme sayaci. +0x02 bayragi kurulu ise +0x08'deki sayac
 *             8'e kadar artiriliyor, doyunca durum 2'ye geciliyor.
 *   durum 2 — ReceiveLinkPackets cagriliyor.
 *
 * Sonra +0x0B kare sayaci artiriliyor ve cagirana tek kelimelik durum
 * ozeti donuluyor: +0x03 dusuk bitler, +0x02 << 8, +0x00 == 8 ise 0x80,
 * hata bayragi icin 0x1000, bekleme sayaci doydu ise 0x8000. 0x2000
 * dali ROM'da ULASILAMAZ (iki bitlik ID alani hicbir zaman 3'ten buyuk
 * olamaz) ama ROM o komutlari uretiyor, o yuzden kaynakta da duruyor.
 *
 * OLCULEN YAZIM KURALLARI (hepsi tek tek denendi; degistirmek eslesmeyi
 * bozuyor):
 *
 *  1. SIOCNT hem 32 bit okunuyor hem +1 baytinda bit temizleniyor; ROM
 *     ikisi icin TEK taban yazmaci tutuyor, o yuzden union gerekli.
 *     Bit 14'u ALAN olarak yazmak sart: bitfield atamasi agbcc'ye maskeyi
 *     32 bitte urettiriyor (`movs #65 / negs`), duz `&= ~0x40` ise
 *     `movs #0xBF` veriyor (2 bayt eksik).
 *  2. Maske sonuclari u8 yerele aliniyor. u32 yerelde agbcc sabiti ve
 *     sonucu ayni sozde-yazmaca birlestirip `movs r0,#0x30 / ands r0,r6`
 *     uretiyor; u8 yerelde sabit QImode dogup regmove'un birlestirmesi
 *     engelleniyor ve ROM'un `movs r1,#0x30 / adds r0,r6,#0 / ands r0,r1`
 *     dizisi cikiyor. Fazladan kesme komutu olusmuyor: maskeler zaten
 *     dusuk bayttan, agbcc daraltmayi eliyor.
 *  3. `status = tmp;` ara atamasi ROM'daki birlesim noktasi kopyasini
 *     (`adds r3,r0,#0`) koruyor; dogrudan atamada agbcc iki dali ortak
 *     son komutta birlestirip 2 bayt kaybettiriyor.
 *  4. `lo` once, `hi` sonra: ROM +0x03'u +0x02'den once okuyor. Dallardaki
 *     VEYA sirasi da olculdu (`0x80 | lo | hi` ve `lo | hi`).
 *  5. TM3CNT ve IF ayri makrolarla yaziliyor; cse ikinci sabit adresi
 *     birincinin yazmacindan +0xF6 olarak tureterek ROM'u veriyor. Elle
 *     isaretci aritmetigi ayni komutlari ama ters yazmaclari uretiyor.
 *  6. Blogun uc bolgesi UC AYRI yerelde tutuluyor; tek degisken ref
 *     sayisini buyutup dagitici sirasini kaydiriyor (r4/r5 takasi).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/link_frame_step.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "comm_block.h"

/* SIOCNT: tek 32 bitlik okuma ve bit 14 (seri kesme izni) icin alan
 * gorunumu; ikisi ayni taban yazmacindan gitmeli (kural 1). */
typedef union SioCtrl {
    u32 word;
    struct {
        u32 low14 : 14;             /* bit 0-13 */
        u32 irq   : 1;              /* bit 14 — seri kesme izni */
        u32 high  : 17;             /* bit 15-31 */
    } bits;
} SioCtrl;

#define SIOC (*(volatile SioCtrl *)REG_SIOCNT_ADDR)

#define SIO_ID_MASK      0x30       /* bit 4-5: coklu oyuncu kimligi */
#define SIO_SD_BUSY      0x88       /* bit 3 SD, bit 7 mesgul */
#define SIO_SD_READY     0x08       /* SD kurulu, mesgul degil */
#define SIO_SI           0x04       /* bit 2 */

#define IE_CLEAR_SERIAL  0xff7f     /* IE'de bit 7'yi silen maske */
#define IE_TIMER3        0x40
#define IF_TIMER3_SERIAL 0xc0
#define TM3_RELOAD_OFF   0xabfb     /* dusuk yari yeniden yukleme,
                                       ust yari denetim = kapali */
#define SEND_LEN         12
#define WAIT_CAP         7

#define ST_IDENT_READY   0x0080
#define ST_ERROR         0x1000
#define ST_ID_HIGH       0x2000

extern u32 ReceiveLinkPackets(u8 *dest);

/* 0x0806660C */
u32 StepLinkFrame(u8 *dest)
{
    CommBlock *setup;
    CommBlock *wait;
    CommBlock *tail;
    u32 cnt;
    u8  id;
    u8  sd;
    u8  si;
    u32 status;
    u32 tmp;
    u32 capped;
    u32 lo;
    u32 hi;

    cnt = SIOC.word;
    setup = gRam02036338;

    switch (setup->byte01) {
    case 0:
        id = cnt & SIO_ID_MASK;
        if (id == 0) {
            sd = cnt & SIO_SD_BUSY;
            if (sd != SIO_SD_READY)
                break;
            si = cnt & SIO_SI;
            if (si == 0 && setup->sendLen == SEND_LEN) {
                REG_IME = 0;
                REG_IE &= IE_CLEAR_SERIAL;
                REG_IE |= IE_TIMER3;
                REG_IME = 1;
                SIOC.bits.irq = 0;
                REG_TM3CNT = TM3_RELOAD_OFF;
                REG_IF = IF_TIMER3_SERIAL;
                setup->byte0 = sd;          /* sd burada SIO_SD_READY */
            }
        }
        gRam02036338->byte01 = 1;
        /* fall through */
    case 1:
        wait = gRam02036338;
        if (wait->byte02 != 0) {
            if (wait->pad08 <= WAIT_CAP)
                wait->pad08++;
            else
                wait->byte01 = 2;
        }
        /* fall through */
    case 2:
        ReceiveLinkPackets(dest);
        break;
    }

    gRam02036338->ident++;

    tail = gRam02036338;
    lo = tail->byte03;
    hi = tail->byte02 << 8;
    if (tail->byte0 == SIO_SD_READY)
        tmp = ST_IDENT_READY | lo | hi;
    else
        tmp = lo | hi;
    status = tmp;

    if (gRam02036338->errorBit != 0)
        status |= ST_ERROR;

    capped = (gRam02036338->pad08 >> 3) << 15;

    /* Iki bitlik ID alani 3'u asamaz; dal ROM'da da olu ama uretiliyor. */
    return (((cnt << 26) >> 30) > 3) ? (ST_ID_HIGH | status | capped)
                                     : (status | capped);
}
