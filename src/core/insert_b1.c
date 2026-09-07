/* Arkaplan katmanlarini kurma — 0x080126E4-0x0801274F
 *
 * Adina ragmen bu fonksiyon liste islemi DEGIL: goruntu denetimini ve uc
 * arkaplan katmanini (BG0/BG1/BG2) kuruyor, sonra bir RAM bayragini
 * sifirliyor. Kardes dosya insert_sorted.c'nin struct yerlesimleri burada
 * kullanilmadi; kalip tumuyle ROM'dan okundu.
 *
 * ROM'un yaptigi (0x080126E4):
 *   DISPCNT = 0x0700            (mod 0; BG0+BG1+BG2 acik)
 *   BG0CNT  = 0x0080 ; |= 0x0004 ; |= 0x1E00
 *   BG1CNT  = 0x0002 ; |= 0x0000 ; |= 0x1D00
 *   BG2CNT  = 0x0081 ; |= 0x0004 ; |= 0x1F00
 *   *(u16 *)0x0201AECC = 0
 *
 * OLCULEN NOKTALAR
 *
 * 1) DISPCNT adresi HAVUZDAN OKUNMUYOR: ROM `movs r1,#128; lsls r1,#19`
 *    uretiyor, yani kaynakta sabit cast (`0x80 << 19`) var, extern sembol
 *    degil. Bu, kural 1'in belgelenmis istisnasi (kaydirmayla kurulabilen
 *    adres); ayni kalip src/world/set_bg1_enable.c'de de olculmustu.
 *
 * 2) BG1CNT ve BG2CNT icin AYRI havuz sabiti YOK: ROM tek bir 0x04000008
 *    literali yukleyip iki kez `adds r2, #2` yapiyor. Bu, kaynakta yuruyucu
 *    isaretci OLDUGU anlamina GELMIYOR -- asagidaki eleme notuna bak.
 *
 * 3) Her katman UC ayri yazma ile kuruluyor: once tam atama, sonra iki
 *    `|=`. BG1'in ilk `|=`'inde ROM'da `ldrh`+`strh` var ama arada `orrs`
 *    yok (0x08012712). Sabit sifir oldugu icin agbcc `| 0`'i eliyor,
 *    volatile erisim oldugu icin yukleme/yazma duruyor. Karakter taban
 *    blogu BG1'de 0 oldugundan kaynakta CHAR_BASE(0) yazildi.
 *
 * 4) 0x0201AECC extern sembol olarak bildirildi (kural 1). Kayit
 *    data/ram_map.csv'ye gRam0201AECC adiyla eklendi; ROM'daki tek erisim
 *    halfword yazma oldugu icin OLCULEN boyut 2 bayttir (kayitta 0 yaziyor,
 *    duzeltilebilir). Kaydin adresi dogrudan havuz sabiti olarak cikiyor,
 *    yani sabit cast yazimiyla bayt bayt ayni.
 *
 * ELENEN YOLLAR (silme, yenilerini ekle)
 *
 *   a) YURUYUCU ISARETCI (`cnt = BG_CNT_BASE; ... cnt++;`), DISPCNT
 *      yazmasindan SONRA atanmis: 104 bayt (4 eksik), 27/54 komut.
 *      agbcc havuz sabitini hic yuklemiyor, cunku DISPCNT icin kurulan
 *      0x04000000 hala yazmacta ve CSE adresi `adds r1, #8` diye
 *      ureterek daha ucuza kapatiyor. Kalan tum fark bu tek katlamadan
 *      dogan yazmac kaymasi.
 *
 *   b) AYNI YURUYUCU ISARETCI, DISPCNT yazmasindan ONCE atanmis: boyut
 *      TUTTU (108) ama havuz yuklemesi fonksiyonun BASINA cikti (kural 19:
 *      agbcc kaynak atamalarini kaynak sirasinda yayiyor), fark 37 bayt.
 *      Yani atama ayri bir deyim oldugu surece ya katlaniyor ya one
 *      kaciyor; ikisinin arasi yok.
 *
 *   c) DISPCNT'i volatile'siz (`*(u16 *)`) yazmak: (a) ile ayni, 104 bayt.
 *      Katlamayi engelleyen sey volatile degil.
 *
 *   d) TUTAN BICIM: isaretci degiskeni HIC YOK; uc yazmac UC AYRI sabit
 *      cast makrosu olarak yaziliyor. Boylece 0x04000008 ilk KULLANIMINDA
 *      havuzdan yukleniyor (one kacmiyor), ve komsu iki adres ondan
 *      `adds #2` ile turetiliyor -- ROM'un tam kalibi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/insert_b1.c
 */

#include "gba_types.h"

/* Goruntu denetimi. Adres kaydirmayla kuruluyor, havuzdan degil. */
#define DISPCNT  (*(vu16 *)(0x80 << 19))

/* Arkaplan denetim yazmaclari. Ayri ayri yazilmalari sart: tek bir
 * yuruyucu isaretci degiskeni farki kapatmiyor (basliktaki (a) ve (b)). */
#define BG0CNT   (*(vu16 *)0x04000008)
#define BG1CNT   (*(vu16 *)0x0400000A)
#define BG2CNT   (*(vu16 *)0x0400000C)

/* Mod 0; BG0, BG1 ve BG2 katmanlari acik. */
#define DISPLAY_MODE  (0xE0 << 3)

/* BGxCNT alanlari. */
#define PRIORITY(n)    (n)
#define CHAR_BASE(n)   ((n) << 2)
#define SCREEN_BASE(n) ((n) << 8)
#define COLOR_256      0x0080

/* Katmanlar kurulurken temizlenen bayrak. ram_map kaydi gerekiyor. */
extern u16 gRam0201AECC;

/* 0x080126E4 */
void ConfigureBgControlRegs(void)
{
    DISPCNT = DISPLAY_MODE;

    BG0CNT = COLOR_256 | PRIORITY(0);
    BG0CNT |= CHAR_BASE(1);
    BG0CNT |= SCREEN_BASE(30);

    BG1CNT = PRIORITY(2);
    BG1CNT |= CHAR_BASE(0);
    BG1CNT |= SCREEN_BASE(29);

    BG2CNT = COLOR_256 | PRIORITY(1);
    BG2CNT |= CHAR_BASE(1);
    BG2CNT |= SCREEN_BASE(31);

    gRam0201AECC = 0;
}
