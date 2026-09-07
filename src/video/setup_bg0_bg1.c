/* BG0/BG1 kurulumu — 0x080127A8-0x080127FB
 *
 * DISPCNT 0x0300 (kip 0, BG0+BG1 acik), BG0CNT 0xC003 | 0x1A00
 * (oncelik 3, boyut 3, ekran tabani 13), BG1CNT 0x4002 | 0x1E00
 * (oncelik 2, boyut 1, ekran tabani 15).
 *
 * IKI SEY BURADA BELIRLEYICI:
 *
 * 1) Denetim yazmaci MUTLAK MAKRO ile yazilmali, isaretci degiskeniyle
 *    DEGIL. Isaretci degiskeni kullanilirsa agbcc 0x04000008'i
 *    0x04000000 + 8 diye ortak altifadeye cikariyor (`adds r1,#8`) ve
 *    havuzdaki 0x04000008 girisi kayboluyor. Mutlak makroda adres bir
 *    MEM adresi olarak kaliyor, sozde-yazmac olmuyor, CSE gormuyor.
 *
 * 2) Yazmaclar `vu16` olmali. include/gba_io.h'daki REG_BG0CNT/REG_BG1CNT
 *    `u16 *` (volatile DEGIL); onlarla ROM'daki geri okuma-yazma ciftleri
 *    (ldrh/strh) tamamen siliniyor. Bu yuzden bu dosya kendi volatile
 *    makrolarini tanimliyor.
 *
 * `BG0CNT_V = BG0CNT_V;` ROM'da gercekten var (ldrh + strh). Deger
 *    degismiyor ama volatile oldugu icin iki veriyolu islemi kaliyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/video/setup_bg0_bg1.c
 */

#include "gba_types.h"
#include "gba_io.h"

#define DISPCNT_VALUE   0x0300
#define BG0CNT_BASE     0xC003
#define BG0CNT_SCREEN   0x1A00
#define BG1CNT_BASE     0x4002
#define BG1CNT_SCREEN   0x1E00

#define BG0CNT_V        (*(vu16 *)0x04000008)
#define BG1CNT_V        (*(vu16 *)0x0400000A)

extern u16 gRam0201AECC;

/* 0x080127A8 */
void SetupBg0Bg1(void)
{
    REG_DISPCNT = DISPCNT_VALUE;

    BG0CNT_V = BG0CNT_BASE;
    BG0CNT_V = BG0CNT_V;
    BG0CNT_V = BG0CNT_V | BG0CNT_SCREEN;

    BG1CNT_V = BG1CNT_BASE;
    BG1CNT_V = BG1CNT_V;
    BG1CNT_V = BG1CNT_V | BG1CNT_SCREEN;

    gRam0201AECC = 0;
}
