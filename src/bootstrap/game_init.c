/* GameInit — 0x08000430-0x0800072F (768 byte)  BYTE-MATCHING
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama: python3 tools/verify_c_function.py src/bootstrap/game_init.c
 *
 * Donanimi ayaga kaldirir (WAITCNT, EWRAM/IWRAM/VRAM/OAM temizleme, kesmeler),
 * sonra iki ic ice sonsuz dongu calistirir: dis dongu bir oturumu kurar, ic
 * dongu her kareyi isler. Ic dongu gLoopState 1 olunca ya da FUN_080664c4
 * sifirdan farkli donunce biter; ardindan ekran sifirlanip dis dongu bastan
 * baslar.
 *
 * -----------------------------------------------------------------------
 * OLCULEN YAZIM KURALLARI (bu fonksiyonu eslesmeye goturen dort ayrinti)
 * -----------------------------------------------------------------------
 *
 * 1. DMA KAYNAK YUVALARI: ROM yigin cercevesi 16 byte ve yuvalar DORT byte
 *    arali (sp+0 word, sp+4 halfword, sp+8 halfword, sp+12 word) — ama sp+4
 *    ve sp+8'e HALFWORD yaziliyor. Duz `u16` degiskenler 2 byte arali
 *    yerlesip cerceveyi 12 bayta dusuruyor (673 fark); `u32` dogru cerceveyi
 *    verip yazimi word yapiyor (264 fark).
 *
 *    Cozum: her yuva `u16 x[2]` DIZISI. Dizi BLKmode oldugu icin agbcc onu
 *    expand_decl aninda, bildirim sirasinda, 4 bayta hizali yerlestiriyor;
 *    `x[0] = 0` yine `strh` uretiyor ve `(u32)x` adresi tek komutta
 *    hesaplaniyor (`mov r0, sp` / `add r0, sp, #4` / `add r6, sp, #8`).
 *    Word yuvasi da dizi olmali (`fillSlot`, `*(u32 *)fillSlot` ile yazilir):
 *    skaler `u32` yazilirsa dizilerden SONRA yerlesip sp+0'i kaybediyor ve
 *    `&fill` artik `mov rX, sp` ile tek komutta uretilemiyor.
 *
 *    Denenip tutmayanlar: struct{u16 h; u16 pad;} (agbcc bunu SImode sayip
 *    `ldr`/`and`/`str` uretiyor), tek buyuk struct (uye erisimi `[sp, #4]`
 *    tabanina donusuyor, ROM'un `add r0,sp,#4` + `[r0,#0]` bicimini
 *    bozuyor), union, u32 yuva + (u16) cast.
 *
 * 2. DMA3 TABAN ISARETCISI UC AYRI YEREL (COMPILER.md kural 17). ROM tabani
 *    uc ayri register'da tutuyor: init'te r4, ic dongude r4 (yeniden
 *    yuklenmis), dis dongunun kapanis blogunda r8. Tek `dma` degiskeni
 *    kullanilirsa omru tum fonksiyona yayiliyor, onceligi dusuyor ve r7'ye
 *    kayiyor — sonra sifir sabiti r4'u kapiyor ve TUM register dagitimi
 *    kayiyor (656/764 fark).
 *
 * 3. DONGU ICI TABAN ATAMALARI DONGUNUN ICINDE YAZILIR (kural 19'un tersi
 *    yonu). `dmaFrame`/`dmaReset` atamalari dongu ONUNE yazilirsa agbcc
 *    onlari kaynak deyimi olarak, derleyicinin kendi urettigi preheader
 *    kopyalarindan ONCE yayiyor:
 *        mov r8,r4 / adds r7,r6,#0 / mov sl,r5     (bizim)
 *        adds r7,r6,#0 / mov sl,r5 / mov r8,r4     (ROM)
 *    Atamalar dongunun icine alininca agbcc'nin dongu-degismezi tasiyicisi
 *    onlari preheader'in SONUNA koyuyor ve sira ROM'unkine oturuyor.
 *    Bu tek degisiklik 11 farkli bayti sifira indirdi.
 *    Onemli: `dmaReset = dma;` (kopya) DEGIL, `= (vu32 *)REG_DMA3_ADDR;`
 *    (sabit) yazilmali — kopya yazilirsa agbcc iki degiskeni birlestirip
 *    tek isaretciye donuyor.
 *
 * 4. DMA kontrol yazmacindan yapilan OLU OKUMA (`dma[2];`) ROM'daki
 *    `ldr r0, [r4, #8]` komutlarinin karsiligidir; her blokta gerekli.
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef signed int     s32;

typedef volatile unsigned short vu16;
typedef volatile unsigned int   vu32;

/* Yazmac adresleri sabit cast olarak yazilir: agbcc hepsini literal havuzdan
 * okuyor, ROM da oyle. (COMPILER.md kural 1'in istisnasi.) */
#define REG_WAITCNT_ADDR 0x04000204
#define REG_DMA3_ADDR    0x040000D4
#define REG_IME_ADDR     0x04000208

#define WAITCNT_BITS 0x4014

/* Hedef adresler: agbcc bunlari kaydirmayla uretiyor (movs #0x80 / lsls #18
 * gibi), ROM'da da oyle. */
#define EWRAM 0x02000000
#define IWRAM 0x03000000
#define VRAM  0x06000000
#define OAM   0x07000000

/* DMA CNT (ust 16 bit kontrol, alt 16 bit transfer sayisi) */
#define DMA_CLEAR_EWRAM  0x85010000   /* 0x10000 word  = 256K EWRAM     */
#define DMA_CLEAR_IWRAM  0x85001F80   /* 0x1F80 word   = 32256 byte     */
#define DMA_CLEAR_VRAM   0x8100C000   /* 0xC000 half   = 96K VRAM       */
#define DMA_FILL_EWRAM   0x85402000
#define DMA_FILL_IWRAM   0x85002000
#define DMA_CLEAR_TAIL   0x85000040   /* 0x40 word = 256 byte IWRAM basi */
#define DMA_CLEAR_OAM    0x81000200   /* 0x200 half = 1K OAM            */

#define EWRAM_FILL 0xCDCDCDCD
#define IWRAM_FILL 0xEFEFEFEF

#define SUBSYSTEM_ARG 0x2FD

extern u8  gVBlankState;
extern u32 gDisplayState;
extern u16 gBiosIrqFlags;
extern u32 gOuterState;
extern u8  gLoopState;
extern u8  gFrameStateSource;
extern u32 gFrameCounterEwram;
extern u32 gFrameReset;
extern u32 gPostFrameState;
extern u8  gGameState[16];

extern void FUN_0806b864(s32 mode);      /* kartus kitapligini baslatir  */
extern void FUN_08005f5c(void);          /* bellek alt sistemi           */
extern void FUN_08063b74(void);          /* DMA3'un bitmesini bekler     */
extern void FUN_0803251c(s32 arg);       /* alt sistem baslatma          */
extern void FUN_0800cae4(void);          /* VBlank bekler                */
extern void InitInterrupts(void);
extern void FUN_08001a00(void);
extern void FUN_080087f4(void);
extern void InitSaveManager(void);
extern void FUN_08005fa8(void);
extern void FUN_0800858c(void);
extern void FUN_08002600(u32 arg);
extern void FUN_080337a8(void);
extern void FUN_0803004c(void);
extern void FUN_0805b1c0(s32 arg);
extern void FUN_08008108(void);
extern s32  FUN_080664c4(void);          /* sifirdan farkli ise dongu biter */
extern void FUN_08013824(void);
extern void FUN_08012248(void);
extern void FUN_08012198(void);
extern void FUN_080100d0(void);
extern void FUN_0800db7c(void);
extern void FUN_08011cf4(void);
extern void FUN_08012a98(void);
extern void FUN_08013098(void);
extern void FUN_08013434(void);
extern void FUN_080138e8(void);
extern void FUN_08014fa0(void);
extern void FUN_08008e68(void);
extern void FUN_08037e7c(void);
extern void FUN_08042784(void);
extern void FUN_08050918(void);
extern void FUN_08051300(void);
extern void FUN_08051960(void);
extern void FUN_08019c04(void);
extern void ResetMenuState(void);
extern void FUN_08061dd4(void);
extern void FUN_080389c0(void);
extern void FUN_080357cc(void);
extern void FUN_0805e118(void);
extern void FUN_08033c94(void);
extern void FUN_0805e168(void);
extern void FUN_0802fe44(void);
extern void FUN_08063d3c(void);

/* 0x08000430 — 768/768 byte BYTE-MATCHING */
void GameInit(void)
{
    vu32 *dma;         /* init blogu           (ROM: r4) */
    vu32 *dmaFrame;    /* ic dongu sifirlamasi (ROM: r4, dongude yeniden yuklenir) */
    vu32 *dmaReset;    /* dis dongu kapanisi   (ROM: r8) */
    vu16 *waitcnt;
    /* Yigin yuvalari — bkz. yukarida (1). Sirasi cerceveyi belirler:
     * sp+0 fillSlot, sp+4 clearSource, sp+8 frameClearSource, sp+12 pending. */
    u16 fillSlot[2];
    u16 clearSource[2];
    u16 frameClearSource[2];
    u32 pending;
    s32 first;
    u8  phase;

    waitcnt = (vu16 *)REG_WAITCNT_ADDR;
    *waitcnt |= WAITCNT_BITS;

    /* EWRAM'i sifirla */
    *(u32 *)fillSlot = 0;
    dma = (vu32 *)REG_DMA3_ADDR;
    dma[0] = (u32)fillSlot;
    dma[1] = EWRAM;
    dma[2] = DMA_CLEAR_EWRAM;
    dma[2];

    /* IWRAM'i sifirla (yigin haric) */
    *(u32 *)fillSlot = 0;
    dma[0] = (u32)fillSlot;
    dma[1] = IWRAM;
    dma[2] = DMA_CLEAR_IWRAM;
    dma[2];

    /* VRAM'i sifirla */
    clearSource[0] = 0;
    dma[0] = (u32)clearSource;
    dma[1] = VRAM;
    dma[2] = DMA_CLEAR_VRAM;
    dma[2];

    FUN_0806b864(1);

    /* Bellegi tanitici desenle doldur (bozuk okuma yakalamak icin) */
    *(u32 *)fillSlot = EWRAM_FILL;
    dma[0] = (u32)fillSlot;
    dma[1] = EWRAM;
    dma[2] = DMA_FILL_EWRAM;
    dma[2];

    *(u32 *)fillSlot = IWRAM_FILL;
    dma[0] = (u32)fillSlot;
    dma[1] = IWRAM;
    dma[2] = DMA_FILL_IWRAM;
    dma[2];

    *(u32 *)fillSlot = 0;
    dma[0] = (u32)fillSlot;
    dma[1] = IWRAM;
    dma[2] = DMA_CLEAR_TAIL;
    dma[2];

    *waitcnt = WAITCNT_BITS;
    FUN_08005f5c();
    FUN_08063b74();

    frameClearSource[0] = 0;
    dma[0] = (u32)frameClearSource;
    dma[1] = VRAM;
    dma[2] = DMA_CLEAR_VRAM;
    dma[2];

    frameClearSource[0] = 0;
    dma[0] = (u32)frameClearSource;
    dma[1] = OAM;
    dma[2] = DMA_CLEAR_OAM;
    dma[2];

    gVBlankState = 1;
    gDisplayState = 0;
    FUN_0803251c(SUBSYSTEM_ARG);
    FUN_0800cae4();
    gBiosIrqFlags |= 1;
    InitInterrupts();
    FUN_08001a00();
    FUN_080087f4();

    pending = 0;

    for (;;) {
        first = 1;
        gOuterState = 0;
        InitSaveManager();
        FUN_08005fa8();
        if (pending == 0)
            FUN_0800858c();
        FUN_08002600(pending);
        pending = 0;
        FUN_080337a8();
        FUN_0803004c();
        FUN_0805b1c0(0);
        FUN_08008108();
        gLoopState = pending;

        do {
            gFrameStateSource = gLoopState;
            gFrameCounterEwram = 0;
            FUN_0800cae4();

            /* Ilk karede ekran zaten kurulu; sonrakilerde yeniden kurulur. */
            if (first == 0) {
                /* Taban atamasi blogun ICINDE — bkz. yukarida (3). */
                dmaFrame = (vu32 *)REG_DMA3_ADDR;
                FUN_08063b74();
                frameClearSource[0] = 0;
                dmaFrame[0] = (u32)frameClearSource;
                dmaFrame[1] = VRAM;
                dmaFrame[2] = DMA_CLEAR_VRAM;
                dmaFrame[2];

                frameClearSource[0] = 0;
                dmaFrame[0] = (u32)frameClearSource;
                dmaFrame[1] = OAM;
                dmaFrame[2] = DMA_CLEAR_OAM;
                dmaFrame[2];

                gVBlankState = 1;
                gDisplayState = 0;
                FUN_0803251c(SUBSYSTEM_ARG);
                FUN_0800cae4();
                gBiosIrqFlags |= 1;
                InitInterrupts();
            }

            gLoopState = 0;
            gFrameReset = 0;
            FUN_08013824();
            FUN_08012248();
            FUN_08012198();
            FUN_080100d0();
            FUN_0800db7c();
            FUN_08011cf4();
            FUN_08012a98();
            FUN_08013098();
            FUN_08013434();
            FUN_080138e8();
            FUN_08014fa0();
            FUN_08008e68();
            FUN_08037e7c();
            FUN_08042784();
            FUN_08050918();
            FUN_08051300();
            FUN_08051960();
            FUN_08019c04();
            ResetMenuState();
            FUN_08061dd4();
            FUN_080389c0();
            FUN_080357cc();
            FUN_0805e118();
            FUN_08033c94();
            FUN_0805e168();
            gPostFrameState = 0;
            FUN_0802fe44();
            FUN_08063d3c();
            *(vu16 *)REG_IME_ADDR = 0;
            FUN_080337a8();
            first = 0;
        } while (gLoopState != 1 && FUN_080664c4() == 0);

        if (FUN_080664c4() != 0)
            pending = 1;

        /* gGameState[12] 1 veya 2 ise sifirlanir (u8 kirpmasi ROM'daki
         * lsls #24 / lsrs #24 ciftini uretir). */
        phase = gGameState[12] - 1;
        if (phase <= 1)
            gGameState[12] = 0;

        /* Taban atamasi dis dongunun ICINDE — bkz. yukarida (3). */
        dmaReset = (vu32 *)REG_DMA3_ADDR;
        FUN_08063b74();
        frameClearSource[0] = 0;
        dmaReset[0] = (u32)frameClearSource;
        dmaReset[1] = VRAM;
        dmaReset[2] = DMA_CLEAR_VRAM;
        dmaReset[2];

        frameClearSource[0] = 0;
        dmaReset[0] = (u32)frameClearSource;
        dmaReset[1] = OAM;
        dmaReset[2] = DMA_CLEAR_OAM;
        dmaReset[2];

        gVBlankState = 1;
        gDisplayState = 0;
        FUN_0803251c(SUBSYSTEM_ARG);
        FUN_0800cae4();
        gBiosIrqFlags |= 1;
        InitInterrupts();
    }
}
