/* Kare isleme zincirlerini calistirir ve tani koyma asamasini kaydeder. */

#include "gba_types.h"

#define gFrameDelay (*(u32 *)0x03000000)

extern volatile u8 gVBlankState;
extern u32 gRam02023700;

extern void RefreshActiveAreas(void);
extern void FUN_080386a8(void);
extern void FUN_0805c190(void);
extern void FUN_080507f4(void);
extern void FUN_08051674(void);
extern void FUN_08040d50(void);
extern void FUN_08050d80(void);
extern void StepNodeLists(void);
extern void FUN_08037a64(void);
extern void FUN_0800d118(void);
extern void MaybeReset(void);
extern void FUN_08013704(void);
extern void FUN_0805eff0(u32 value);
extern void FUN_08035318(void);
extern void FUN_0806262c(void);
extern void FUN_08030290(void);

extern void FUN_08008f14(void);
extern void FUN_08038530(void);
extern void FUN_08025cbc(void);
extern void FUN_08037ee8(void);
extern void FUN_08035848(void);
extern void FUN_0800d660(void);
extern void FrameChain(void);
extern void FUN_08029e44(u32 value, u32 zero);
extern void FUN_0805e28c(u32 value);
extern void NoOp08035CD4(void);
extern void FUN_0806020c(void);
extern void FUN_08014fc4(void);
extern void SortActiveSprites(void);

/* 0x0805135C */
void RunFrameStageOne(void)
{
    /* Ana dunya zincirinin ilk yarisi. Isaretler kilitlenme aninda son
     * tamamlanan asamayi RAM'den okumayi saglar. */
    RefreshActiveAreas();
    gRam02023700 = 0x65;
    FUN_080386a8();
    gRam02023700 = 0x66;
    FUN_0805c190();
    gRam02023700 = 0x67;
    FUN_080507f4();
    FUN_08051674();
    gRam02023700 = 0x68;
    FUN_08040d50();
    FUN_08050d80();
    gRam02023700 = 0x69;
    StepNodeLists();
    gRam02023700 = 0x6A;
    FUN_08037a64();
    gRam02023700 = 0x6B;
    FUN_0800d118();
    gRam02023700 = 0x6C;
    MaybeReset();
    FUN_08013704();
    /* 0x03000000 VBlank tarafindan yazilan kare gecikmesidir. */
    FUN_0805eff0(gFrameDelay);
    gRam02023700 = 0x73;
    FUN_08035318();
    gRam02023700 = 0xDD;
    FUN_0806262c();
    gRam02023700 = 0xDE;
    FUN_08030290();
    gRam02023700 = 0xDF;
}

/* 0x080513E0 */
void RunFrameStageTwo(void)
{
    u32 marker;

    /* Volatile okuma ROM'da korunmus; degerin kendisi bu zincirde kullanilmaz. */
    (void)gVBlankState;
    gRam02023700 = 0x6D;
    FUN_08008f14();
    FUN_08038530();
    FUN_08025cbc();
    gRam02023700 = 0x6E;
    FUN_08037ee8();
    gRam02023700 = 0x6F;
    FUN_08035848();
    gRam02023700 = 0x71;
    FUN_0800d660();
    marker = 0x72;
    gRam02023700 = marker;
    FrameChain();
    gRam02023700 = marker;
    /* Iki ardil adim ayni VBlank gecikme degerini tuketir. */
    FUN_08029e44(gFrameDelay, 0);
    FUN_0805e28c(gFrameDelay);
    gRam02023700 = 0x74;
    NoOp08035CD4();
    gRam02023700 = 0x75;
    FUN_0806020c();
    FUN_08014fc4();
    gRam02023700 = 0x76;
    SortActiveSprites();
}
