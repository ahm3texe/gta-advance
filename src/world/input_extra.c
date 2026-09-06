/* Baglanti kablosu bekleme ekrani — 0x0806620C-0x0806634B
 *
 * Iki metin satiri cizip iki ayri bekleme dongusune giriyor. Ilk dongu
 * her karede VBlank bekleyip KEYINPUT'un 1. bitini (B) izliyor: bit once
 * SET gorulunce (tus BIRAKILMIS) 0. bit (A) armed'a yaziliyor; sonra bit
 * CLEAR olunca (B basili) gBiosIrqFlags'in 0. biti temizlenip 1 donuyor.
 * A hala basili degilse dongu suruyor, aksi halde ikinci ekrana geciliyor.
 *
 * Ikinci dongu ResetLinkSession sonrasi ayni tus mantigini kosturuyor ama
 * her turda FUN_080657d8(0) ile suruculeri adimliyor ve MaybeAdvance ile
 * ayni kosulu (gVBlankEnabled == 2 && gRam0200048C > 1) satir ici
 * tekrarliyor; kosul saglaninca dongu bitip 0 donuyor. Iptal yolunda
 * gBiosIrqFlags maskeleniyor, ResetLinkHardware cagriliyor, oturum durumu
 * sifirlaniyor ve 1 donuyor.
 *
 * DURUM: PARK — 12/320 bayt farkli (%96.3). Kalan fark TEK SINIF:
 * dongu ONCESI (preheader) yapilan degismez-ifade kaldirmalarinin SIRASI.
 * Uretilen komutlarin hepsi ayni, yazmac atamalari da ayni; yalnizca iki
 * yerde iki komutun (ve buna bagli literal havuz sozcuklerinin) sirasi
 * ters:
 *
 *   0x08066238  ROM: mov r6,#2 / ldr r7,=gBiosIrqFlags / mov r4,#1
 *               biz: mov r6,#2 / mov r4,#1 / ldr r7,=gBiosIrqFlags
 *   0x080662C4  ROM: ldr r7,=gVBlankEnabled / ldr r6,=gBiosIrqFlags
 *               biz: ldr r6,=gBiosIrqFlags / ldr r7,=gVBlankEnabled
 *               (havuz sozcukleri de ayni sirayla yer degistiriyor)
 *
 * OLCULEN MEKANIZMA: kaldirma sirasi kaynak metnindeki KULLANIM sirasini
 * izliyor; kaynak dizilisi ise ROM'un blok yerlesimini zorunlu kiliyor ve
 * ikisi celisiyor:
 *   - `if (armed == 0) {B} else {A}` -> ROM'un blok yerlesimi DOGRU
 *     (cmp/bne A, B ortada, A sonda) ama sira 2,1,flags.
 *   - `if (armed != 0) {A} else {B}` -> sira 2,flags,1 (ROM) ama derleyici
 *     B'yi dongunun ustune tasiyip giris dali ekliyor: 316 bayt, yerlesim
 *     bozuluyor.
 *
 * DENENIP ELENENLER (hepsi olculdu):
 *   - Dongu 2 icin `while (again)` + `if (!again) break` (WaitLinkSettle
 *     bicimi): fazladan bir `cmp/beq` kaliyor ve `again` cagri boyunca
 *     yasadigi icin callee-saved yazmac tutuyor -> 328 bayt, 124 fark.
 *   - Dongu 2 icin do/while'a `goto` ile girmek: dogru kuyruk, ama dongu
 *     dogal olmadigi icin adres kaldirmalari tamamen kayboluyor (324/235).
 *   - Dongu 2 govdesini `if (again != 0) {...} continue; break;` ile
 *     sarmak: 316, dondurme kayboluyor.
 *   - Dongu 1 icin bfirst/bfirst+continue/bfirst+goto/else-if: dordu de
 *     ayni ikiliyi uretiyor (12).
 *   - `gBiosIrqFlags = gBiosIrqFlags & 0xFFFE`, `&= ~1`, armed u32/u16,
 *     again u32, held s32, sabiti sola alma, extern bildirim sirasi:
 *     hicbiri sirayi degistirmiyor (12).
 *   - Isaretci/yerel degisken ile ELLE kaldirma (`bit1 = 2; irq =
 *     &gBiosIrqFlags;`) dongu 1'i TAM eslestiriyor (8 bayta iniyor):
 *     kaynak duzeyi degiskenlerin baslatmasi loop.c'nin kaldirmalarindan
 *     ONCE yaziliyor. Ayni numara dongu 2'de basarisiz: serbest kalan
 *     yazmac yuzunden sabit 2 de kaldiriliyor, r8'e tasma olusuyor
 *     (336/344). Uydurma degisken oldugu icin REDDEDILDI — 8 bayt icin
 *     savunulamayan kaynak yazmaktansa temiz kaynakla 12 bayt.
 *
 * PERMUTER KOSTURULDU (1222 yineleme): temel 80 -> en iyi 20, sifir yok.
 * En iyi aday REDDEDILDI: VBlankIntrWait'i iki kez cagiriyor (davranis
 * degisikligi), `held` degiskenini alakasiz bir deger icin yeniden
 * kullaniyor ve `do{...}while(0)` sarmalayicisi ekliyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/input_extra.c
 */

#include "gba_types.h"
#include "gba_io.h"

#define MSG_LINK_WAIT_1   0x1C9
#define MSG_LINK_WAIT_2   0x1CA
#define MSG_LINK_BUSY_1   0x1C6
#define MSG_LINK_BUSY_2   0x1C8

#define TEXT_X            120
#define TEXT_Y_2          32

#define KEY_BIT1          2
#define KEY_BIT0          1
#define BIOS_IRQ_KEEP     0xFFFE
#define LINK_STATE_LIVE   2

extern u16 gVBlankEnabled;
extern u16 gRam0200048C;

extern void FUN_08006124(void);
extern u32  GetTextString(u32 index);
extern void DrawTextCentred(u32 text, s32 x, s32 y);
extern void VBlankIntrWait(void);
extern void ResetLinkSession(void);
extern void ResetLinkHardware(void);
extern void FUN_080657d8(s32 code);

/* 0x0806620C */
s32 WaitForPartner(void)
{
    s32 armed;
    s32 again;
    u16 held;

    armed = 0;

    FUN_08006124();
    DrawTextCentred(GetTextString(MSG_LINK_WAIT_1), TEXT_X, 0);
    DrawTextCentred(GetTextString(MSG_LINK_WAIT_2), TEXT_X, TEXT_Y_2);
    VBlankIntrWait();

    for (;;) {
        VBlankIntrWait();

        if (armed == 0) {
            if (REG_KEYINPUT & KEY_BIT1)
                armed = REG_KEYINPUT & KEY_BIT0;
        } else {
            if ((REG_KEYINPUT & KEY_BIT1) == 0) {
                gBiosIrqFlags &= BIOS_IRQ_KEEP;
                return 1;
            }
            if ((REG_KEYINPUT & KEY_BIT0) == 0)
                break;
        }
    }

    VBlankIntrWait();
    FUN_08006124();
    DrawTextCentred(GetTextString(MSG_LINK_BUSY_1), TEXT_X, 0);
    DrawTextCentred(GetTextString(MSG_LINK_BUSY_2), TEXT_X, TEXT_Y_2);
    ResetLinkSession();
    VBlankIntrWait();

    armed = 0;
    for (;;) {
        FUN_080657d8(0);

        again = 1;
        if (gVBlankEnabled == LINK_STATE_LIVE && gRam0200048C > 1)
            again = 0;
        if (again == 0)
            break;

        if (armed != 0) {
            VBlankIntrWait();
            held = REG_KEYINPUT & KEY_BIT1;
            if (held == 0) {
                gBiosIrqFlags &= BIOS_IRQ_KEEP;
                ResetLinkHardware();
                gVBlankEnabled = held;
                return 1;
            }
        } else {
            if (REG_KEYINPUT & KEY_BIT1)
                armed = REG_KEYINPUT & KEY_BIT0;
        }
    }

    VBlankIntrWait();
    return 0;
}
