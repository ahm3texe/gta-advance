/* DURUM: PARK — 2356/2374 bayt, 575/1174 KOMUT ayni (onceki tur: 553/1174).
 *
 * OLCUT KOMUT SAYISIDIR, BAYT DEGIL. Bayt sayisini kapatan degisiklik
 * yanlis olabilir; boyutu buyuten degisiklik dogru olabilir.
 *   make c-match FILE=src/world/sio_driver.c
 *   python3 tools/diff_function.py src/world/sio_driver.c FUN_080657d8
 *
 * ------------------------------------------------------------------
 * BU TURDA COZULEN: TAKMA-AD (ALIAS) SINIFI  [553 -> 568]
 * ------------------------------------------------------------------
 * Kare kaydinin alanlari once duz `*(u16 *)((u8 *)b + ofs)` ile
 * yaziliyordu. gcc 2.x'in takma-ad cozumlemesinde yapi DISI bir u16
 * yazmasi, yapi DISI bir skaler global okumasini gecersiz kiliyor;
 * bu yuzden ROM'un TEK KEZ hesapladigi `gRam0200048C & 31`
 *     movs r0,#31 / ldr r1,=0200048c / ldrh r1,[r1] / ands r0,r1
 * bizde SLOT_INDEX yazmasindan sonra IKINCI kez hesaplaniyordu.
 * Alanlar `LinkFrame` yapisinin uyeleri yapilinca (COMPONENT_REF ->
 * MEM_IN_STRUCT_P) yazma artik o okumayi oldurmuyor ve ROM'un bicimi
 * cikiyor. Ofsetler blok tabanina gore (0x180..0x19F) verildigi icin
 * Thumb'in strh anlik alanina sigmiyor, dolayisiyla ROM gibi her
 * erisimde sabit yazmaca kuruluyor -- ara `LinkSlot *` degiskeninin
 * urettigi `strh [r,#2]` bicimi olusmuyor.
 *
 * ------------------------------------------------------------------
 * BU TURDA COZULEN: BOLGE BASINA AYRI YEREL (kural 59)  [568 -> 575]
 * ------------------------------------------------------------------
 *   - TX kaydinin ilk yarisi `blk`, ikinci yarisi AYRI bir `tx2`
 *     isaretcisi. ROM ikisi icin farkli yazmac kullaniyor (r3 / r5),
 *     yani kaynakta da iki degisken var. Tek degisken kullanmak
 *     `blk`i iki yariya birden canli tutup r3'u bosa cikariyordu.
 *     (`tx`i yeniden kullanmak: 569 -- ayri degisken sart.)
 *   - Bolum 2'nin pencere degiskenleri (`end2/hi2/start2/prev2/i2/j2`)
 *     Bolum 1'inkilerden ayri. Paylasmak referans sayisini sisiriyor.
 *
 * ------------------------------------------------------------------
 * ILK KALAN SAPMA: 0x08065834
 * ------------------------------------------------------------------
 *   ROM : mov r7, sp / ldrh r7,[r7] / strh r7,[r3]
 *   biz : mov r0, sp / ldrh r0,[r0] / strh r0,[r3]
 * Dort `mov rN,sp / ldrh / strh` ucluSUNUN dorduncusu. Saf yerel
 * dagitim; ilk uc (r4/r5/r6) ROM ile ayni.
 *
 * ------------------------------------------------------------------
 * KOK SORUN (COZULMEDI): TUM GOVDE BIR YAZMAC KAYMIS
 * ------------------------------------------------------------------
 * Uzun omurlu degerlerin dagitimi ROM'a gore bir yukari kaymis:
 *     ROM : k=r4  blk=r5  cur=r6  (&gRam02000420 kopyasi=r7)
 *     biz : k=r5  blk=r6  cur=r7  (&gRam02000420 kopyasi=r3)
 * Sayilari ayni; tek fark bizim `k` pseudo'sunun DONANIM r4 ile
 * cakismasi (`dump_alloc --conflicts`: `38 conflicts ... 0 1 2 3 4 13`).
 * Cakisma, yerel dagiticinin (local-alloc, global'den ONCE calisir)
 * blok 79'da -- TX kaydinin ikinci yarisi, `k`nin canli oldugu yer --
 * &gRam02000E80'i r4'e koymasindan geliyor. ROM ayni blokta r3'u
 * ikinci kez kullanip (once 0x020003c0, sonra 0x02000e80) r4'u bos
 * birakiyor. Bu tek cakisma cozulurse govdenin buyuk bolumu hizalanir;
 * yuzlerce komutluk fark bundan.
 *
 * ------------------------------------------------------------------
 * DENENIP ELENEN YAZIMLAR
 * ------------------------------------------------------------------
 *   RX kaydi `*(u16*)(b + 0x190 + f + i*16)`  (yapi disi)      -> 568
 *   RX kaydi `(*(LinkSlot *)(b + 0x190 + i*16)).alan`          -> 519
 *   RX kaydi `((LinkSlot *)(b + 0x190))[i].alan`               -> 524
 *   RX kaydi `FRAME(b)->rx[i].alan`                     <- SECILDI 575
 *     (RX'te ROM'un adres birlesimi `(b+sabit)+i*16`; bizimki
 *      `(b+i*16)+sabit`. Bizimki daha kisa cikiyor ama yapi bicimi
 *      genel toplamda yine de en iyisi -- iki gereksinim carpisiyor.)
 *   TX'in ikinci yarisinda `blk`i yeniden kullanmak                569
 *   TX'in ikinci yarisinda `tx`i (READY'nin degiskeni) kullanmak   569
 *   `do {...} while (t <= 240)` yerine `while (t <= 240) {...}`    575 (fark yok)
 *   Pencere testini ters cevirmek:
 *     `if ((hi-cur)&31 > (end-cur)&31) start=cur; else {...}`      566
 *     (ROM'un blok yerlesimini taklit etmek icin denendi; agbcc
 *      bunun yerine `start`i secim gibi uretip yigina tasiyor.)
 *
 * ------------------------------------------------------------------
 * BLOK DURUMU (tools/dump_cfg.py, 103 blok)
 * ------------------------------------------------------------------
 * Komut komut AYNI: B0-B3, B5, B8, B11-B14, B16-B18, B86, B88
 *   (giris blogu + case 0/IDLE'in tamami + case 1/READY'nin buyuk
 *    kismi). B4 53/57, B15 48/52, B19 74/91.
 * Geri kalan: LIVE dongusunun govdesi -- farkin tamami orada.
 * NOT: diff dogrusal hizaladigi icin ilk sapmadan sonraki blok
 * eslesmeleri YAKLASIKTIR.
 *
 * Sinir dogru (tek prolog/epilog), atlama tablosu YOK, gereken tum RAM
 * sembolleri data/ram_map.csv'de. `asm(".equ ...")` KULLANILMAZ.
 */

/* Baglanti (SIO) surucusunun ana dagiticisi — 0x080657D8-0x0806611D
 *
 * Yapi: tek `switch (gVBlankEnabled)`; dort durum.
 *   0 IDLE     : tus durumunu kopyalar, kenar maskelerini uretir, sifirlar.
 *   1 READY    : StepLinkFrame ile el sikisir; (status & 3) == 3 olunca
 *                yuva numarasini SIOCNT'ten cikarip LIVE'a gecer.
 *   2 LIVE     : asil dongu -- her turda StepLinkFrame, karsi tarafin
 *                iki penceresini (son ve guncel) halka tamponuna isler,
 *                onaya kadar yurur, kendi kaydini kurup gonderir.
 *                240 kareyi asarsa SETTLING'e duser.
 *   3 SETTLING : IDLE ile ayni temizlik.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 */

#include "gba_types.h"
#include "gba_io.h"
#include "comm_block.h"
#include "game_state.h"

#define RING_MASK     31
#define WINDOW_HALF   15
#define HOLD_SPAN      6
#define LINK_TIMEOUT 240
#define ACK_NONE      32
#define SLOT_MAGIC   0xDEAD

#define LINK_STATE_IDLE     0
#define LINK_STATE_READY    1
#define LINK_STATE_LIVE     2
#define LINK_STATE_SETTLING 3

/* Iletisim blogunun icindeki kare kayitlari. Gonderilecek kayit +0x180'de,
 * alinan kayitlar +0x190'dan itibaren oyuncu basina 16 bayt. */
#define LINK_TX_OFS 0x180
#define LINK_RX_OFS 0x190

/* Kare kaydinin alanlari. Erisim BLOK TABANINDAN yapiliyor: ofsetler
 * (0x180..0x19F) Thumb'in strh anlik alanina (0..62) sigmadigi icin ROM her
 * erisimde sabiti yazmaca kurup taban ile topluyor. Ara `LinkSlot *`
 * degiskeni kurmak taban adresi tek sefer hesaplatip `strh [r,#2]`
 * uretiyor -- o yuzden her erisim `FRAME(b)->tx.alan` seklinde.
 *
 * Alanlarin YAPI UYESI olarak yazilmasi sart: gcc 2.x'in takma-ad
 * cozumlemesinde yapi icindeki bir yazma, yapi disi bir skaler global
 * okumasini (gRam0200048C) gecersiz KILMIYOR; duz `*(u16 *)` yazmasi ise
 * kiliyor ve ROM'da tek kez hesaplanan `gRam0200048C & 31` bizde iki kez
 * hesaplaniyordu. */
typedef struct LinkSlot {
    u16 index;                          /* +0x00 */
    u16 keys;                           /* +0x02 */
    u16 ack;                            /* +0x04 */
    u16 endIndex;                       /* +0x06 */
    u16 endKeys;                        /* +0x08 */
    u16 tag;                            /* +0x0A */
    u16 endTag;                         /* +0x0C */
    u16 magic;                          /* +0x0E */
} LinkSlot;

typedef struct LinkFrame {
    u8       pad000[LINK_TX_OFS];
    LinkSlot tx;                        /* +0x180 */
    LinkSlot rx[2];                     /* +0x190 */
} LinkFrame;

#define FRAME(b)      ((LinkFrame *)(b))

/* Karsi tarafin bu karedeki kaydi. ROM her erisimde hem blok isaretcisini
 * hem de yuva numarasini YENIDEN okuyor; makro bunu koruyor. */
#define RX_SLOT       (FRAME(gRam02036338)->rx[gRam020004A4])


extern u16 gVBlankEnabled;              /* 0x02000D08 */
extern u16 gRam0200048C;
extern u16 gRam02000498;

/* IWRAM'daki uc kelime bitisik; ROM birini digerinden `adds #8` / `adds #4`
 * ile turetiyor, yani derleme aninda ARALARINDAKI FARK BILINIYOR. Sembol
 * olarak yazilirsa bu tureme olusmuyor, o yuzden adres sabiti. */
#define gRam03000098 (*(u32 *)0x03000098)
#define gRam0300009C (*(u32 *)0x0300009C)
#define gRam030000A0 (*(u32 *)0x030000A0)
extern u16 gRam02000230[];
extern u16 gRam02000420[];
extern u8  gRam02000100[];
extern u8  gRam02000140[];
extern u8  gRam020003C0[];
extern u8  gRam02000400[];
extern u8  gRam02000E80[];
extern u32 gRam020110B8;
extern u32 gFrameCounterLate;
extern u8  gRam02036328;
extern s16 gSlotSelector;               /* 0x02000D40 */

/* Bu yedi sembol data/ram_map.csv'de YOK. Adresleri burada `.equ` ile
 * veriliyor; sembol referansi kalmasi SART, adres sabiti yazmak cse'ye
 * ("ldr" yerine "adds rX,#fark") sahte tureme yaptiriyor. Haritaya
 * eklendiginde bu blok silinip yalnizca `extern` bildirimleri kalmali. */

extern u16 gRam02000134;
extern u16 gRam02000288;
extern u16 gRam020003EC;
extern s16 gRam020004A4;
extern u16 gRam020004C0;
extern u16 gRam02000D0C;
extern u32 gRam02036324;

extern u32  FUN_0806c2d4(u32 counter, u32 kind);
extern void MaybeReset(void);
extern void PollInput(void);
extern u32  StepLinkFrame(u8 *dest);
extern void MaybeSetCommByte6(void);
extern void BuildLinkPacket(const void *payload);

/* 0x080657D8 */
void FUN_080657d8(u32 mode)
{
    CommBlock *blk;
    /* Kaydin ikinci yarisi (end* alanlari) icin AYRI isaretci: ROM ilk
     * yariyi bir yazmacta (r3), ikinci yariyi baskasinda (r5) tutuyor,
     * yani kaynakta iki degisken var. Tek degisken kullanmak ikisini de
     * canli tutup dagitimi bir yazmac kaydiriyor. */
    CommBlock *tx2;
    CommBlock *tx;
    u32 status;
    u32 id;
    u32 step;
    u32 same;
    s32 base;
    s32 cur;
    s32 end;
    s32 end2;    /* Bolum 2 kendi yerellerini kullanir (kural 59) */
    s32 hi;
    s32 hi2;
    s32 start;
    s32 start2;
    s32 prev;
    s32 prev2;
    s32 i;
    s32 i2;
    s32 j;
    s32 j2;
    s32 k;
    s32 startTime;
    u8 *slot;
    u16 prevA;
    u16 prevB;
    u16 keysA;
    u16 keysB;

    if (mode != LINK_STATE_LIVE) {
        gGameState.word00++;
        if (gVBlankEnabled != 0 && mode == 0
                && FUN_0806c2d4(gGameState.word00, 5) != 0) {
            gRam0300009C = gRam03000098 = mode;
            gRam02036330.half00 = mode;
            gRam02000498 = gRam020003EC = gRam02000D0C = gRam02000134 = mode;
            gRam030000A0 = gRam020004C0 = mode;
            MaybeReset();
            return;
        }
        PollInput();
    }

    switch (gVBlankEnabled) {
    case LINK_STATE_IDLE:
        gGameState.pad06  = gGameState.pressed;
        gGameState.half04 = gGameState.held;
        gRam03000098 = gGameState.half04 & ~gRam0300009C;
        gRam030000A0 = gRam0300009C & ~gGameState.half04;
        gRam0300009C = gGameState.half04;
        gRam02036330.half00 = 0;
        gRam02036330.half02 = 0;
        gRam02000498 = gRam0300009C;
        gRam02000D0C = gRam03000098;
        gRam020004C0 = gRam020003EC = gRam02000134 = 0;
        break;

    case LINK_STATE_READY:
        gRam02036324 = StepLinkFrame((u8 *)FRAME(gRam02036338)->rx);
        MaybeSetCommByte6();
        if ((gRam02036324 & 3) == 3) {
            id = (REG_SIOCNT32 << 26) >> 30;
            if (id <= 1) {
                gSlotSelector  = id;
                gRam020004A4   = 1 - id;
                gVBlankEnabled = LINK_STATE_LIVE;
                gRam02000288   = 2;
                gGameState.word00 = 1;
            }
        }
        gGameState.pad06  = gGameState.pressed;
        gGameState.half04 = gGameState.held;
        gRam03000098 = gGameState.half04 & ~gRam0300009C;
        gRam030000A0 = gRam0300009C & ~gGameState.half04;
        gRam0300009C = gGameState.half04;
        gRam02036330.half00 = 0;
        gRam02036330.half02 = 0;

        tx = gRam02036338;
        FRAME(tx)->tx.index    = 0;
        FRAME(tx)->tx.keys     = 0;
        FRAME(tx)->tx.ack      = ACK_NONE;
        FRAME(tx)->tx.endIndex = 0;
        FRAME(tx)->tx.endKeys  = 0;
        BuildLinkPacket(&FRAME(tx)->tx);

        gRam02000498 = gRam0300009C;
        gRam02000D0C = gRam03000098;
        gRam020004C0 = gRam020003EC = gRam02000134 = 0;
        break;

    case LINK_STATE_LIVE:
        k = gRam0200048C & RING_MASK;
        if (mode == LINK_STATE_LIVE)
            base = k;
        else
            base = (gRam0200048C - 1) & RING_MASK;

        cur = base;
        while (gRam02000100[cur] != 0)
            cur = (cur + 1) & RING_MASK;

        startTime = gFrameCounterLate;
        slot = &gRam02000100[base];

        do {
            status = StepLinkFrame((u8 *)FRAME(gRam02036338)->rx);
            gRam02036324 = status;
            if (((1 << gRam020004A4) & status) != 0
                    && RX_SLOT.magic == SLOT_MAGIC) {
                /* Bolum 1: karsi tarafin "son" penceresi. */
                if (RX_SLOT.index != RX_SLOT.endIndex) {
                    end = RX_SLOT.endIndex & RING_MASK;
                    if (((end - cur) & RING_MASK) <= WINDOW_HALF) {
                        hi = (RX_SLOT.endTag >> 8) & RING_MASK;
                        if (((hi - cur) & RING_MASK) <= ((end - cur) & RING_MASK)) {
                            start = hi;
                            prev = (cur - 1) & RING_MASK;
                            if (((gRam02000400[prev] + 1) & RING_MASK)
                                    == (RX_SLOT.endTag & RING_MASK)) {
                                i = cur;
                                if (i != hi) {
                                    do {
                                        gRam02000230[i] = gRam02000230[prev];
                                        gRam02000100[i] = 1;
                                        gRam02000400[i] = gRam02000400[prev];
                                        gRam02000140[i] = gRam02000140[prev];
                                        i = (i + 1) & RING_MASK;
                                    } while (i != hi);
                                }
                            }
                        } else {
                            start = cur;
                        }

                        j = start;
                        for (;;) {
                            gRam02000230[j] = RX_SLOT.endKeys;
                            gRam02000100[j] = 2;
                            gRam02000400[j] = RX_SLOT.endTag;
                            gRam02000140[j] = RX_SLOT.endTag >> 8;
                            if (j == end)
                                break;
                            j = (j + 1) & RING_MASK;
                        }

                        while (gRam02000100[cur] != 0)
                            cur = (cur + 1) & RING_MASK;
                    }
                }

                /* Bolum 2: karsi tarafin bu karedeki penceresi. */
                end2 = RX_SLOT.index & RING_MASK;
                if (((end2 - cur) & RING_MASK) <= WINDOW_HALF) {
                    hi2 = (RX_SLOT.tag >> 8) & RING_MASK;
                    if (((hi2 - cur) & RING_MASK) <= ((end2 - cur) & RING_MASK)) {
                        start2 = hi2;
                        prev2 = (cur - 1) & RING_MASK;
                        if (((gRam02000400[prev2] + 1) & RING_MASK)
                                == (RX_SLOT.tag & RING_MASK)) {
                            i2 = cur;
                            if (i2 != hi2) {
                                do {
                                    gRam02000230[i2] = gRam02000230[prev2];
                                    gRam02000100[i2] = 3;
                                    gRam02000400[i2] = gRam02000400[prev2];
                                    gRam02000140[i2] = gRam02000140[prev2];
                                    i2 = (i2 + 1) & RING_MASK;
                                } while (i2 != hi2);
                            }
                        }
                    } else {
                        start2 = cur;
                    }

                    j2 = start2;
                    for (;;) {
                        gRam02000230[j2] = RX_SLOT.keys;
                        gRam02000100[j2] = 4;
                        gRam02000400[j2] = RX_SLOT.tag;
                        gRam02000140[j2] = RX_SLOT.tag >> 8;
                        if (j2 == end2)
                            break;
                        j2 = (j2 + 1) & RING_MASK;
                    }

                    while (gRam02000100[cur] != 0)
                        cur = (cur + 1) & RING_MASK;
                }

                /* Bolum 3: karsi tarafin onayladigi noktadan ileri yuru. */
                if (RX_SLOT.ack == ACK_NONE) {
                    k = gRam0200048C & RING_MASK;
                } else {
                    k = RX_SLOT.ack & RING_MASK;
                    if (((gRam0200048C - k) & RING_MASK) > WINDOW_HALF) {
                        k = gRam0200048C & RING_MASK;
                    } else {
                        step = gRam020003C0[k & RING_MASK];
                        same = (step == gRam020003C0[(k - 1) & RING_MASK]);
                        while (k != (s32)(gRam0200048C & RING_MASK)) {
                            if (same
                                    && gRam020003C0[(k + 1) & RING_MASK] - step > 1)
                                break;
                            if (gRam020003C0[k & RING_MASK]
                                    != gRam020003C0[(k + 1) & RING_MASK])
                                break;
                            k = (k + 1) & RING_MASK;
                        }
                    }
                }
            }

            /* Yerel taraf onaya yetismisse geriye dogru sabit bandi ara. */
            if ((k & RING_MASK) == (s32)(gRam0200048C & RING_MASK)) {
                step = gRam020003C0[k & RING_MASK];
                for (;;) {
                    k = (k - 1) & RING_MASK;
                    if (gRam020003C0[k] != step)
                        break;
                    if (((gRam0200048C - k) & RING_MASK) > HOLD_SPAN) {
                        k = gRam0200048C & RING_MASK;
                        break;
                    }
                }
            }

            blk = gRam02036338;
            if (blk->ready == 0) {
                FRAME(blk)->tx.index = gRam0200048C & RING_MASK;
                FRAME(blk)->tx.keys  = gRam02000420[gRam0200048C & RING_MASK];
                if (*slot != 0 || mode == LINK_STATE_LIVE)
                    FRAME(blk)->tx.ack = ACK_NONE;
                else
                    FRAME(blk)->tx.ack = cur;

                tx2 = gRam02036338;
                FRAME(tx2)->tx.endIndex = k & RING_MASK;
                FRAME(tx2)->tx.endKeys  = gRam02000420[k & RING_MASK];
                FRAME(tx2)->tx.tag =
                      (gRam02000E80[gRam0200048C & RING_MASK] << 8)
                    |  gRam020003C0[gRam0200048C & RING_MASK];
                FRAME(tx2)->tx.endTag =
                      (gRam02000E80[k & RING_MASK] << 8)
                    |  gRam020003C0[k & RING_MASK];
                FRAME(tx2)->tx.magic = SLOT_MAGIC;
                BuildLinkPacket(&FRAME(tx2)->tx);
            }

            if (mode == LINK_STATE_LIVE)
                return;

            if (*slot != 0 || gRam0200048C <= 1) {
                if (gRam0200048C > 1) {
                    prevA = gGameState.half04;
                    prevB = gRam02036330.half02;
                    keysB = gRam02000230[base];
                    gRam02036330.half02 = keysB;
                    gRam02036330.half00 = (prevB ^ keysB) & keysB;
                    keysA = gRam02000420[base];
                    gGameState.half04 = keysA;
                    gGameState.pad06  = (prevA ^ keysA) & keysA;

                    if (gSlotSelector == 0) {
                        gRam03000098 = gGameState.pad06;
                        gRam030000A0 = gRam0300009C & ~gGameState.half04;
                        gRam0300009C = gGameState.half04;
                        gRam02000134 = gRam02036330.half00;
                        gRam020004C0 = gRam020003EC & ~keysB;
                        gRam020003EC = keysB;
                    } else {
                        gRam03000098 = gRam02036330.half00;
                        gRam030000A0 = gRam0300009C & ~gRam02036330.half02;
                        gRam0300009C = gRam02036330.half02;
                        gRam02000134 = gGameState.pad06;
                        gRam020004C0 = gRam020003EC & ~keysA;
                        gRam020003EC = keysA;
                    }
                } else {
                    gRam02036330.half02 = 0;
                    gGameState.half04 = 0;
                    gRam0300009C = 0;
                    gRam03000098 = 0;
                    gRam020003EC = 0;
                    gRam02000134 = 0;
                }

                gRam020110B8 += (gRam02036330.half02 + gGameState.half04)
                                << (gRam0200048C & 15);
                gRam02000498 = gRam0300009C | gRam020003EC;
                gRam02000D0C = gRam03000098 | gRam02000134;
                gRam02000100[base] = 0;
                return;
            }
        } while ((s32)(gFrameCounterLate - startTime) <= LINK_TIMEOUT);

        gVBlankEnabled = LINK_STATE_SETTLING;
        gRam02036328 = 1;
        break;

    case LINK_STATE_SETTLING:
        gGameState.pad06  = gGameState.pressed;
        gGameState.half04 = gGameState.held;
        gRam03000098 = gGameState.half04 & ~gRam0300009C;
        gRam030000A0 = gRam0300009C & ~gGameState.half04;
        gRam0300009C = gGameState.half04;
        gRam02036330.half00 = 0;
        gRam02036330.half02 = 0;
        gRam02000498 = gRam0300009C;
        gRam02000D0C = gRam03000098;
        gRam020004C0 = gRam020003EC = gRam02000134 = 0;
        break;
    }
}
