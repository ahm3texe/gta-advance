/* Baglanti oturumu kapanis ve bekleme adimlari — 0x080663E8-0x080664A3
 *
 * Uc adim. Birincisi surucuye 15 kare boyunca 2 kodunu verip oturumu
 * kapatiyor, ikincisi durum 3'te ekrani karartip 60 kare bekliyor,
 * ucuncusu durum stabil olana ya da 240 kare gecene kadar surucuyu
 * 0 koduyla adimliyor.
 *
 * gVBlankEnabled adi yaniltici: burada 0/2/3 alan bir OTURUM DURUMU.
 * Ad diger eslesmis kaynaklarda kullanildigi icin degistirilmedi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/link_session.c
 */

#include "gba_types.h"
#include "gba_io.h"

#define LINK_STATE_SETTLING 2
#define LINK_STATE_FAILED   3
#define FADE_FRAMES         60
#define SETTLE_FRAMES       240
#define BLEND_ALL           0x3FFF
#define BLEND_FULL          16

extern u16 gVBlankEnabled;
extern u8  gRam02036328;
extern u16 gRam0200048C;

extern void FUN_080657d8(s32 code);
extern void ResetLinkHardware(void);
extern void VBlankIntrWait(void);

/* 0x080663E8 */
void ShutdownLinkSession(s32 drain)
{
    s32 i;

    if (drain != 0) {
        i = 14;
        do {
            FUN_080657d8(2);
            VBlankIntrWait();
            i--;
        } while (i >= 0);
    }

    ResetLinkHardware();
    gVBlankEnabled = 0;
}

/* 0x08066414 */
s32 FadeOutIfLinkState3(void)
{
    s32 i;

    if (gVBlankEnabled != LINK_STATE_FAILED)
        return 0;

    REG_BLDCNT = BLEND_ALL;
    REG_BLDY = BLEND_FULL;

    i = FADE_FRAMES - 1;
    do {
        VBlankIntrWait();
        i--;
    } while (i >= 0);

    return 1;
}

/* 0x08066454 */
void WaitLinkSettle(void)
{
    s32 frames;
    s32 again;

    /* Dongu bicimi ROM'dan okundu (kural 49). `for (;;)` + `if (!again)
     * break` yazimi kontrol blogunu basa, bekleme blogunu sona koyuyor ve
     * gRam02036328 adresini dongu disina tasiyor; ROM'da sira ters ve
     * adres blogun icinde yukleniyor. Bayragi dongudan ONCE kurup
     * `while (again)` yazmak dogru rotasyonu veriyor. */
    frames = 0;
    again = 1;
    while (again != 0) {
        FUN_080657d8(0);

        again = 1;
        if (gVBlankEnabled == LINK_STATE_SETTLING && gRam0200048C > 1)
            again = 0;
        if (again == 0)
            break;

        VBlankIntrWait();
        frames++;
        if (frames > SETTLE_FRAMES - 1) {
            gVBlankEnabled = LINK_STATE_FAILED;
            gRam02036328 = 1;
            break;
        }
    }
}
