/* GameInit — 0x08000430-0x0800072F (768 bytes)  BYTE-MATCHING
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification: python3 tools/verify_c_function.py src/bootstrap/game_init.c
 *
 * Brings the hardware up (WAITCNT, clearing EWRAM/IWRAM/VRAM/OAM, interrupts),
 * then runs two nested infinite loops: the outer loop sets up a session, the
 * inner loop processes each frame. The inner loop ends when gLoopState becomes
 * 1 or IsSessionActive returns nonzero; the display is then reset and the
 * outer loop starts again.
 *
 * -----------------------------------------------------------------------
 * MEASURED SOURCE-FORM RULES (the four details that brought this function
 * to a match)
 * -----------------------------------------------------------------------
 *
 * 1. DMA SOURCE SLOTS: the ROM's stack frame is 16 bytes and the slots are
 *    FOUR bytes apart (sp+0 words, sp+4 halfwords, sp+8 halfwords, sp+12 words)
 *    -- yet sp+4 and sp+8 are written as HALFWORDS. Plain `u16` variables lay
 *    out 2 bytes apart and shrink the frame to 12 bytes (673 differences);
 *    `u32` gives the right frame but makes the store a word (264 differences).
 *
 *    Solution: each slot is a `u16 x[2]` ARRAY. Because an array is BLKmode,
 *    agbcc lays it out 4-byte aligned at expand_decl time, in declaration
 *    order; `x[0] = 0` still emits `strh` and the address `(u32)x` is computed
 *    in a single instruction (`mov r0, sp` / `add r0, sp, #4` /
 *    `add r6, sp, #8`).
 *    The word slot must be an array too (`fillSlot`, written through
 *    `*(u32 *)fillSlot`): a scalar `u32` is laid out AFTER the arrays, loses
 *    sp+0, and `&fill` can no longer be produced by a single `mov rX, sp`.
 *
 *    Tried and rejected: struct{u16 h; u16 pad;} (agbcc treats it as SImode
 *    and emits `ldr`/`and`/`str`), one large struct (member access turns into
 *    a `[sp, #4]` base and breaks the ROM's `add r0,sp,#4` + `[r0,#0]` form),
 *    a union, and a u32 slot with a (u16) cast.
 *
 * 2. THE DMA3 BASE POINTER IS THREE SEPARATE LOCALS (COMPILER.md rule 17).
 *    The ROM keeps the base in three separate registers: r4 during init, r4
 *    again (reloaded) in the inner loop, and r8 in the outer loop's closing
 *    block. With a single `dma` variable its lifetime spans the whole
 *    function, its priority drops and it moves to r7 -- then the zero constant
 *    takes r4 and the ENTIRE register allocation shifts (656/764 differences).
 *
 * 3. THE IN-LOOP BASE ASSIGNMENTS ARE WRITTEN INSIDE THE LOOP (the inverse
 *    direction of rule 19). If the `dmaFrame`/`dmaReset` assignments are
 *    written BEFORE the loop, agbcc emits them as source statements, ahead of
 *    the preheader copies it generates itself:
 *        mov r8,r4 / adds r7,r6,#0 / mov sl,r5     (ours)
 *        adds r7,r6,#0 / mov sl,r5 / mov r8,r4     (ROM)
 *    Moving the assignments inside the loop makes agbcc's loop-invariant
 *    motion place them at the END of the preheader, and the order matches the
 *    ROM's. That single change took 11 differing bytes to zero.
 *    Important: it must be written `= (vu32 *)REG_DMA3_ADDR;` (the constant),
 *    NOT `dmaReset = dma;` (a copy) -- with a copy agbcc coalesces the two
 *    variables into a single pointer.
 *
 * 4. The DEAD READ of the DMA control register (`dma[2];`) corresponds to the
 *    `ldr r0, [r4, #8]` instructions in the ROM; it is needed in every block.
 */

#include "gba_io.h"

/* Register addresses are written as constant casts: agbcc reads all of them
 * from the literal pool, and so does the ROM. (The exception to COMPILER.md
 * rule 1.) */

#define WAITCNT_BITS 0x4014

/* Destination addresses: agbcc produces these by shifting (such as
 * movs #0x80 / lsls #18), and so does the ROM. */
#define EWRAM 0x02000000
#define IWRAM 0x03000000
#define VRAM  0x06000000
#define OAM   0x07000000

/* DMA CNT (upper 16 bits control, lower 16 bits transfer count) */
#define DMA_CLEAR_EWRAM  0x85010000   /* 0x10000 word  = 256K EWRAM     */
#define DMA_CLEAR_IWRAM  0x85001F80   /* 0x1F80 word   = 32256 bytes     */
#define DMA_CLEAR_VRAM   0x8100C000   /* 0xC000 half   = 96K VRAM       */
#define DMA_FILL_EWRAM   0x85402000
#define DMA_FILL_IWRAM   0x85002000
#define DMA_CLEAR_TAIL   0x85000040   /* 0x40 words = 256 bytes at the start of IWRAM */
#define DMA_CLEAR_OAM    0x81000200   /* 0x200 half = 1K OAM            */

#define EWRAM_FILL 0xCDCDCDCD
#define IWRAM_FILL 0xEFEFEFEF

#define SUBSYSTEM_ARG 0x2FD

extern volatile u8 gVBlankState;
extern u32 gDisplayState;
extern u32 gOuterState;
extern u8  gLoopState;
extern u8  gFrameStateSource;
extern u32 gFrameCounterEwram;
extern u32 gFrameReset;
extern u32 gPostFrameState;
extern u8  gGameState[16];

extern void RegisterRamReset(s32 flags);     /* BIOS swi 1: RAM/IO reset */
extern void FUN_08005f5c(void);          /* the memory subsystem         */
extern void FUN_08063b74(void);          /* waits for DMA3 to finish     */
extern void FUN_0803251c(s32 arg);       /* subsystem start-up           */
extern void FUN_0800cae4(void);          /* waits for VBlank             */
extern void InitInterrupts(void);
extern void RunMenuLoop(void);
extern void FUN_080087f4(void);
extern void InitSaveManager(void);
extern void FUN_08005fa8(void);
extern void FUN_0800858c(void);
extern void FUN_08002600(u32 arg);
extern void StopAudioDmaOnCartFlag(void);
extern void FUN_0803004c(void);
extern void FUN_0805b1c0(s32 arg);
extern void ZeroHistory(void);
extern s32  IsSessionActive(void);          /* nonzero ends the loop */
extern void FUN_08013824(void);
extern void FUN_08012248(void);
extern void FUN_08012198(void);
extern void InitWorkBuffers(void);
extern void NoOp0800DB7C(void);
extern void FUN_08011cf4(void);
extern void ForwardToInitNodePool(void);
extern void FUN_08013098(void);
extern void FUN_08013434(void);
extern void FUN_080138e8(void);
extern void FUN_08014fa0(void);
extern void FUN_08008e68(void);
extern void FUN_08037e7c(void);
extern void FUN_08042784(void);
extern void ResetAnchorSmall(void);
extern void ResetRecordIndex(void);
extern void NoOp08051960(void);
extern void NoOp08019C04(void);
extern void ResetMenuState(void);
extern void FUN_08061dd4(void);
extern void FUN_080389c0(void);
extern void FUN_080357cc(void);
extern void FUN_0805e118(void);
extern void FUN_08033c94(void);
extern void FUN_0805e168(void);
extern void FUN_0802fe44(void);
extern void FUN_08063d3c(void);

/* 0x08000430 — 768/768 bytes BYTE-MATCHING */
void GameInit(void)
{
    vu32 *dma;         /* the init block       (ROM: r4) */
    vu32 *dmaFrame;    /* inner-loop clear (ROM: r4, reloaded in the loop) */
    vu32 *dmaReset;    /* outer-loop shutdown  (ROM: r8) */
    vu16 *waitcnt;
    /* Stack slots -- see (1) above. Their order determines the frame:
     * sp+0 fillSlot, sp+4 clearSource, sp+8 frameClearSource, sp+12 pending. */
    u16 fillSlot[2];
    u16 clearSource[2];
    u16 frameClearSource[2];
    u32 pending;
    s32 first;
    u8  phase;

    waitcnt = (vu16 *)REG_WAITCNT_ADDR;
    *waitcnt |= WAITCNT_BITS;

    /* Clear EWRAM */
    *(u32 *)fillSlot = 0;
    dma = (vu32 *)REG_DMA3_ADDR;
    dma[0] = (u32)fillSlot;
    dma[1] = EWRAM;
    dma[2] = DMA_CLEAR_EWRAM;
    dma[2];

    /* Clear IWRAM (excluding the stack) */
    *(u32 *)fillSlot = 0;
    dma[0] = (u32)fillSlot;
    dma[1] = IWRAM;
    dma[2] = DMA_CLEAR_IWRAM;
    dma[2];

    /* Clear VRAM */
    clearSource[0] = 0;
    dma[0] = (u32)clearSource;
    dma[1] = VRAM;
    dma[2] = DMA_CLEAR_VRAM;
    dma[2];

    RegisterRamReset(1);

    /* Fill memory with an identifying pattern (to catch corrupt reads) */
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
    RunMenuLoop();
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
        StopAudioDmaOnCartFlag();
        FUN_0803004c();
        FUN_0805b1c0(0);
        ZeroHistory();
        gLoopState = pending;

        do {
            gFrameStateSource = gLoopState;
            gFrameCounterEwram = 0;
            FUN_0800cae4();

            /* On the first frame the display is already set up; on later ones it is rebuilt. */
            if (first == 0) {
                /* The base assignment is INSIDE the block -- see (3) above. */
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
            InitWorkBuffers();
            NoOp0800DB7C();
            FUN_08011cf4();
            ForwardToInitNodePool();
            FUN_08013098();
            FUN_08013434();
            FUN_080138e8();
            FUN_08014fa0();
            FUN_08008e68();
            FUN_08037e7c();
            FUN_08042784();
            ResetAnchorSmall();
            ResetRecordIndex();
            NoOp08051960();
            NoOp08019C04();
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
            StopAudioDmaOnCartFlag();
            first = 0;
        } while (gLoopState != 1 && IsSessionActive() == 0);

        if (IsSessionActive() != 0)
            pending = 1;

        /* gGameState[12] is cleared when it is 1 or 2 (the u8 truncation
         * produces the ROM's lsls #24 / lsrs #24 pair). */
        phase = gGameState[12] - 1;
        if (phase <= 1)
            gGameState[12] = 0;

        /* The base assignment is INSIDE the outer loop -- see (3) above. */
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
