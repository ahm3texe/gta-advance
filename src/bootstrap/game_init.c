/* Oyun baslatma ve ana dongu — 0x08000430-0x0800072F
 *
 * Bellek bolgelerini DMA3 ile temizler ve doldurur, alt sistemleri baslatir,
 * sonra ic ice iki dongu calistirir: dis dongu bir oturumu, ic dongu tek tek
 * kareleri surer.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/bootstrap/game_init.c
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef signed int     s32;

typedef struct {
    const void *src;
    void *dst;
    u32 control;
} DmaChannel;

#define REG_WAITCNT (*(volatile u16 *)0x04000204)
#define REG_IME     (*(volatile u16 *)0x04000208)
#define REG_DMA3    (*(volatile DmaChannel *)0x040000D4)

#define WAITCNT_BITS 0x4014

#define EWRAM ((void *)0x02000000)
#define IWRAM ((void *)0x03000000)
#define VRAM  ((void *)0x06000000)
#define OAM   ((void *)0x07000000)

#define DMA_CLEAR_EWRAM      0x85010000
#define DMA_CLEAR_IWRAM      0x85001F80
#define DMA_CLEAR_VRAM       0x8100C000
#define DMA_CLEAR_OAM        0x81000200
#define DMA_FILL_EWRAM       0x85402000
#define DMA_FILL_IWRAM       0x85002000
#define DMA_CLEAR_IWRAM_TAIL 0x85000040

#define EWRAM_FILL_VALUE 0xCDCDCDCD
#define IWRAM_FILL_VALUE 0xEFEFEFEF

#define SUBSYSTEM_ARGUMENT 0x2FD

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

extern void FUN_0806b864(u32 mode);
extern void FUN_08005f5c(void);
extern void FUN_08063b74(void);
extern void FUN_0803251c(u32 argument);
extern void FUN_0800cae4(void);
extern void InitInterrupts(void);
extern void FUN_08001a00(void);
extern void FUN_080087f4(void);
extern void InitSaveManager(void);
extern void FUN_08005fa8(void);
extern void FUN_0800858c(void);
extern void FUN_08002600(s32 pending);
extern void FUN_080337a8(void);
extern void FUN_0803004c(void);
extern void FUN_0805b1c0(u32 argument);
extern void FUN_08008108(void);
extern s32  FUN_080664c4(void);
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

/* 0x08000430 — HENUZ ESLESMIYOR (772 byte'in ~508'i tutuyor)
 *
 * Kontrol akisi dogru cikarildi ve fonksiyonun bas kismi (ilk ~35 komut)
 * birebir eslesiyor. Kalan fark register/yigin dagitiminda.
 *
 * Olculen: yigin degiskeni tipi buyuk fark yaratiyor --
 *   u16 kaynaklar (volatile degil): 673 fark
 *   u16 kaynaklar (volatile):       590 fark
 *   u32 kaynaklar:                  264 fark   <- mevcut
 *   u16 dizi [2]:                   304 fark
 *   u32 yuva + (u16) cast yazim:    739 fark
 * ROM yigin cercevesi 16 byte (sub sp, #16) ve `pending` sp+12'de tutuluyor;
 * u32 kaynaklarla bu yerlesim yakalandi.
 *
 * Kalan bilinen sapma: ROM DMA kaynagina 16 bit yaziyor (strh), bizimki
 * 32 bit (str). Yuva 4 byte aralikli olmali AMA yazim halfword olmali;
 * denenen iki bicim de bu ikisini ayni anda vermedi.
 *
 * Blok tamamlanana kadar src/bootstrap/game_init.s gecerli build
 * kaynagidir. */
void GameInit(void)
{
    u32 fill;
    u32 clearSource;
    u32 frameClearSource;
    s32 pending;
    s32 firstFrame;
    s32 frameFlag;

    REG_WAITCNT |= WAITCNT_BITS;

    fill = 0;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = EWRAM;
    REG_DMA3.control = DMA_CLEAR_EWRAM;
    REG_DMA3.control;

    fill = 0;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = IWRAM;
    REG_DMA3.control = DMA_CLEAR_IWRAM;
    REG_DMA3.control;

    clearSource = 0;
    REG_DMA3.src = &clearSource;
    REG_DMA3.dst = VRAM;
    REG_DMA3.control = DMA_CLEAR_VRAM;
    REG_DMA3.control;

    FUN_0806b864(1);

    fill = EWRAM_FILL_VALUE;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = EWRAM;
    REG_DMA3.control = DMA_FILL_EWRAM;
    REG_DMA3.control;

    fill = IWRAM_FILL_VALUE;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = IWRAM;
    REG_DMA3.control = DMA_FILL_IWRAM;
    REG_DMA3.control;

    fill = 0;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = IWRAM;
    REG_DMA3.control = DMA_CLEAR_IWRAM_TAIL;
    REG_DMA3.control;

    REG_WAITCNT = WAITCNT_BITS;
    FUN_08005f5c();
    FUN_08063b74();

    frameClearSource = 0;
    REG_DMA3.src = &frameClearSource;
    REG_DMA3.dst = VRAM;
    REG_DMA3.control = DMA_CLEAR_VRAM;
    REG_DMA3.control;

    frameClearSource = 0;
    REG_DMA3.src = &frameClearSource;
    REG_DMA3.dst = OAM;
    REG_DMA3.control = DMA_CLEAR_OAM;
    REG_DMA3.control;

    gVBlankState = 1;
    gDisplayState = 0;
    FUN_0803251c(SUBSYSTEM_ARGUMENT);
    FUN_0800cae4();
    gBiosIrqFlags |= 1;
    InitInterrupts();
    FUN_08001a00();
    FUN_080087f4();

    pending = 0;

    for (;;) {
        firstFrame = 1;
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
        frameFlag = 0;

        for (;;) {
            gFrameStateSource = gLoopState;
            gFrameCounterEwram = frameFlag;
            FUN_0800cae4();

            if (firstFrame == 0) {
                FUN_08063b74();

                frameClearSource = 0;
                REG_DMA3.src = &frameClearSource;
                REG_DMA3.dst = VRAM;
                REG_DMA3.control = DMA_CLEAR_VRAM;
                REG_DMA3.control;

                frameClearSource = 0;
                REG_DMA3.src = &frameClearSource;
                REG_DMA3.dst = OAM;
                REG_DMA3.control = DMA_CLEAR_OAM;
                REG_DMA3.control;

                gVBlankState = 1;
                gDisplayState = 0;
                FUN_0803251c(SUBSYSTEM_ARGUMENT);
                FUN_0800cae4();
                gBiosIrqFlags |= 1;
                InitInterrupts();
            }

            gLoopState = frameFlag;
            gFrameReset = frameFlag;

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

            gPostFrameState = frameFlag;
            FUN_0802fe44();
            FUN_08063d3c();
            REG_IME = frameFlag;
            FUN_080337a8();

            firstFrame = 0;
            if (gLoopState == 1)
                break;
            if (FUN_080664c4())
                break;
        }

        if (FUN_080664c4())
            pending = 1;

        if ((u8)(gGameState[12] - 1) <= 1)
            gGameState[12] = 0;

        FUN_08063b74();

        frameClearSource = 0;
        REG_DMA3.src = &frameClearSource;
        REG_DMA3.dst = VRAM;
        REG_DMA3.control = DMA_CLEAR_VRAM;
        REG_DMA3.control;

        frameClearSource = 0;
        REG_DMA3.src = &frameClearSource;
        REG_DMA3.dst = OAM;
        REG_DMA3.control = DMA_CLEAR_OAM;
        REG_DMA3.control;

        gVBlankState = 1;
        gDisplayState = 0;
        FUN_0803251c(SUBSYSTEM_ARGUMENT);
        FUN_0800cae4();
        gBiosIrqFlags |= 1;
        InitInterrupts();
    }
}
