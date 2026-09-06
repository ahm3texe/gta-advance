/* DURUM: PARK — 2372/2374 bayt AMA yalnizca 532/1174 KOMUT ayni.
 *
 * Boyutun iki bayt yakin olmasi YANILTICI: govdenin yarisindan
 * fazlasi hala yanlis. Olcut komut dizisi, bayt sayisi degil.
 *
 * Bu turda cozulenler (ajanin birakti'gi 2280 -> derlenebilir 2372):
 *   - 7 eksik RAM sembolu data/ram_map.csv'ye eklendi; tipleri ROM'dan
 *     okundu (alti strh = u16, biri str/ldr = u32).
 *   - gRam02036330 tur catismasi cozuldu: LinkCounters artik
 *     include/comm_block.h'de, +0x00 ve +0x02 alanlariyla. ROM o
 *     adrese yalnizca ofset 0 ve 2'den yaziyor (dogrulandi).
 *   - gRam02000230 iki dosyada da u16[] yapildi.
 *
 * ILK SAPMA girisin hemen ardinda: ROM mod degerini bir dizi globale
 * belirli bir SIRAYLA yaziyor ve bizde olmayan fazladan bir
 * `mov r4,sp / ldrh r4 / strh r4,[r0]` blogu var. Buradan baslamak
 * gerekiyor -- ilk blok duzelmeden asagisi hizalanmaz.
 *
 * Yapisal harita: tools/dump_cfg.py FUN_080657d8 (103 blok, atlama
 * tablosu YOK, tek prolog/epilog -- sinir dogru).
 * Olcum: make c-match FILE=src/world/sio_driver.c
 *        python3 tools/diff_function.py src/world/sio_driver.c FUN_080657d8
 */

/* Baglanti (SIO) surucusunun ana dagiticisi — 0x080657D8-0x0806611D
 *
 * DURUM: CALISMA HALINDE (bkz. dosya sonundaki HARITA/OLCUM notu).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/sio_driver.c
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

#define SLOT_INDEX     0x00
#define SLOT_KEYS      0x02
#define SLOT_ACK       0x04
#define SLOT_ENDINDEX  0x06
#define SLOT_ENDKEYS   0x08
#define SLOT_TAG       0x0A
#define SLOT_ENDTAG    0x0C
#define SLOT_MAGICF    0x0E

/* Alanlara ISARETCI UZERINDEN degil, blok + BAYT OFSETI olarak erisiliyor:
 * ofsetler Thumb'in ldrh/strh anlik alanina (0..62) sigmadigi icin ROM her
 * erisimde sabiti yazmaca kurup topluyor. Ara `LinkSlot *` degiskeni
 * kurmak taban adresi tek sefer hesaplatip `strh [r,#2]` uretiyor. */
#define TX(b, f)      (*(u16 *)((u8 *)(b) + LINK_TX_OFS + (f)))
#define RX(b, i, f)   (*(u16 *)((u8 *)(b) + LINK_RX_OFS + (f) + (i) * 16))

/* Karsi tarafin bu karedeki kaydi. ROM her erisimde hem blok isaretcisini
 * hem de yuva numarasini YENIDEN okuyor; makro bunu koruyor. */
#define RX_SLOT(f) RX(gRam02036338, gRam020004A4, f)

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
    CommBlock *tx;
    u8 *slot;
    u32 status;
    u32 id;
    u32 step;
    u32 same;
    s32 base;
    s32 cur;
    s32 end;
    s32 hi;
    s32 start;
    s32 prev;
    s32 i;
    s32 j;
    s32 k;
    s32 startTime;
    u16 prevA;
    u16 prevB;
    u16 keysA;
    u16 keysB;

    if (mode != LINK_STATE_LIVE) {
        gGameState.word00++;
        if (gVBlankEnabled != 0 && mode == 0
                && FUN_0806c2d4(gGameState.word00, 5) != 0) {
            gRam02036330.half00 = gRam0300009C = gRam03000098 = mode;
            gRam02000498 = gRam020003EC = gRam02000D0C = gRam02000134 = mode;
            gRam020004C0 = mode;
            gRam030000A0 = mode;
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
        gRam02036324 = StepLinkFrame((u8 *)gRam02036338 + LINK_RX_OFS);
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
        TX(tx, SLOT_INDEX)    = 0;
        TX(tx, SLOT_KEYS)     = 0;
        TX(tx, SLOT_ACK)      = ACK_NONE;
        TX(tx, SLOT_ENDINDEX) = 0;
        TX(tx, SLOT_ENDKEYS)  = 0;
        BuildLinkPacket((u8 *)tx + LINK_TX_OFS);

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
            status = StepLinkFrame((u8 *)gRam02036338 + LINK_RX_OFS);
            gRam02036324 = status;
            if (((1 << gRam020004A4) & status) != 0
                    && RX_SLOT(SLOT_MAGICF) == SLOT_MAGIC) {
                /* Bolum 1: karsi tarafin "son" penceresi. */
                if (RX_SLOT(SLOT_INDEX) != RX_SLOT(SLOT_ENDINDEX)) {
                    end = RX_SLOT(SLOT_ENDINDEX) & RING_MASK;
                    if (((end - cur) & RING_MASK) <= WINDOW_HALF) {
                        hi = (RX_SLOT(SLOT_ENDTAG) >> 8) & RING_MASK;
                        if (((hi - cur) & RING_MASK) <= ((end - cur) & RING_MASK)) {
                            start = hi;
                            prev = (cur - 1) & RING_MASK;
                            if (((gRam02000400[prev] + 1) & RING_MASK)
                                    == (RX_SLOT(SLOT_ENDTAG) & RING_MASK)) {
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
                            gRam02000230[j] = RX_SLOT(SLOT_ENDKEYS);
                            gRam02000100[j] = 2;
                            gRam02000400[j] = RX_SLOT(SLOT_ENDTAG);
                            gRam02000140[j] = RX_SLOT(SLOT_ENDTAG) >> 8;
                            if (j == end)
                                break;
                            j = (j + 1) & RING_MASK;
                        }

                        while (gRam02000100[cur] != 0)
                            cur = (cur + 1) & RING_MASK;
                    }
                }

                /* Bolum 2: karsi tarafin bu karedeki penceresi. */
                end = RX_SLOT(SLOT_INDEX) & RING_MASK;
                if (((end - cur) & RING_MASK) <= WINDOW_HALF) {
                    hi = (RX_SLOT(SLOT_TAG) >> 8) & RING_MASK;
                    if (((hi - cur) & RING_MASK) <= ((end - cur) & RING_MASK)) {
                        start = hi;
                        prev = (cur - 1) & RING_MASK;
                        if (((gRam02000400[prev] + 1) & RING_MASK)
                                == (RX_SLOT(SLOT_TAG) & RING_MASK)) {
                            i = cur;
                            if (i != hi) {
                                do {
                                    gRam02000230[i] = gRam02000230[prev];
                                    gRam02000100[i] = 3;
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
                        gRam02000230[j] = RX_SLOT(SLOT_KEYS);
                        gRam02000100[j] = 4;
                        gRam02000400[j] = RX_SLOT(SLOT_TAG);
                        gRam02000140[j] = RX_SLOT(SLOT_TAG) >> 8;
                        if (j == end)
                            break;
                        j = (j + 1) & RING_MASK;
                    }

                    while (gRam02000100[cur] != 0)
                        cur = (cur + 1) & RING_MASK;
                }

                /* Bolum 3: karsi tarafin onayladigi noktadan ileri yuru. */
                if (RX_SLOT(SLOT_ACK) == ACK_NONE) {
                    k = gRam0200048C & RING_MASK;
                } else {
                    k = RX_SLOT(SLOT_ACK) & RING_MASK;
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
                TX(blk, SLOT_INDEX) = gRam0200048C & RING_MASK;
                TX(blk, SLOT_KEYS)  = gRam02000420[gRam0200048C & RING_MASK];
                if (*slot != 0 || mode == LINK_STATE_LIVE)
                    TX(blk, SLOT_ACK) = ACK_NONE;
                else
                    TX(blk, SLOT_ACK) = cur;

                blk = gRam02036338;
                TX(blk, SLOT_ENDINDEX) = k & RING_MASK;
                TX(blk, SLOT_ENDKEYS)  = gRam02000420[k & RING_MASK];
                TX(blk, SLOT_TAG) =
                      (gRam02000E80[gRam0200048C & RING_MASK] << 8)
                    |  gRam020003C0[gRam0200048C & RING_MASK];
                TX(blk, SLOT_ENDTAG) =
                      (gRam02000E80[k & RING_MASK] << 8)
                    |  gRam020003C0[k & RING_MASK];
                TX(blk, SLOT_MAGICF) = SLOT_MAGIC;
                BuildLinkPacket((u8 *)blk + LINK_TX_OFS);
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
