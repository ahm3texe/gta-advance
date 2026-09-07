/* Arka plan kaydirma ve afin donusum yazmaclarini sifirlama
 * 0x080034EC-0x0800355B  (112 bayt, ESLESTI)
 *
 * Kesme kapatmadan, cagri yapmadan yalnizca 0x04000010-0x0400003F araligina
 * yaziyor: dort arka planin H/V kaydirmasi sifirlaniyor, BG2/BG3 afin
 * matrisleri birim matrise (pa = pd = 0x100, pb = pc = 0) ve referans
 * noktalari sifira cekiliyor. Yaprak fonksiyon, `bx lr` ile donuyor:
 * yigin cercevesi yok, donus tipi void (kural 35).
 *
 * ROM tek literal yukluyor (0x04000012 = BG0VOFS) ve butun otekilere
 * `adds`/`subs` ile yuruyor; yani adresler kaynakta sabit ifade olarak
 * yaziliyor ve CSE onlari tek tabana bagliyor.
 *
 * YAZMA SIRASI ROM'DAN OKUNDU, tahmin degil (erisimler volatile oldugu icin
 * sira korunuyor):
 *   1) her BG icin ONCE VOFS sonra HOFS, BG0'dan BG3'e
 *   2) referans noktalari BG2/BG3 DONUSUMLU: X alt, X ust, Y alt, Y ust
 *   3) matris katsayilari yine BG2/BG3 donusumlu: pa, pb, pc, pd
 *
 * DENENEN TEK YAZIM ILK DENEMEDE TUTTU (112/112). Not olarak: burada kural 1
 * GECERLI DEGIL -- adresler extern sembol yapilmadi, `(vu16 *)0x040000xx`
 * sabit cast olarak birakildi. ROM'un tek literal + `adds`/`subs` yuruyusu
 * tam olarak sabit ifadenin isaretidir; extern sembole cevirmek tabani
 * havuzdan ayri ayri okuturdu (docs/COMPILER.md, "kural 1 evrensel degildir").
 *
 * `volatile` sart (kural 4/12): niteleme kalkinca agbcc ard arda gelen
 * olu store'lari eleyip fonksiyonu bosaltir. AFFINE_ONE'in `movs r3,#128 /
 * lsls r3,#1 / adds r2,r3,#0` diye UC komutta kurulmasi -- yani fazladan
 * yazmac kopyasi -- kaynaktan zorlanmadi, dagiticidan kendiliginden cikti.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/cutscene_b1.c
 */

#include "gba_types.h"

/* Kaydirma yazmaclari (yalniz yazilir). */
#define REG_BG0HOFS (*(vu16 *)0x04000010)
#define REG_BG0VOFS (*(vu16 *)0x04000012)
#define REG_BG1HOFS (*(vu16 *)0x04000014)
#define REG_BG1VOFS (*(vu16 *)0x04000016)
#define REG_BG2HOFS (*(vu16 *)0x04000018)
#define REG_BG2VOFS (*(vu16 *)0x0400001A)
#define REG_BG3HOFS (*(vu16 *)0x0400001C)
#define REG_BG3VOFS (*(vu16 *)0x0400001E)

/* BG2 afin matrisi ve referans noktasi. X/Y 28.4 sabit noktali 32 bit,
 * ROM iki halfword olarak yazdigi icin burada da alt/ust ayri. */
#define REG_BG2PA   (*(vu16 *)0x04000020)
#define REG_BG2PB   (*(vu16 *)0x04000022)
#define REG_BG2PC   (*(vu16 *)0x04000024)
#define REG_BG2PD   (*(vu16 *)0x04000026)
#define REG_BG2X_L  (*(vu16 *)0x04000028)
#define REG_BG2X_H  (*(vu16 *)0x0400002A)
#define REG_BG2Y_L  (*(vu16 *)0x0400002C)
#define REG_BG2Y_H  (*(vu16 *)0x0400002E)

/* BG3 afin matrisi ve referans noktasi. */
#define REG_BG3PA   (*(vu16 *)0x04000030)
#define REG_BG3PB   (*(vu16 *)0x04000032)
#define REG_BG3PC   (*(vu16 *)0x04000034)
#define REG_BG3PD   (*(vu16 *)0x04000036)
#define REG_BG3X_L  (*(vu16 *)0x04000038)
#define REG_BG3X_H  (*(vu16 *)0x0400003A)
#define REG_BG3Y_L  (*(vu16 *)0x0400003C)
#define REG_BG3Y_H  (*(vu16 *)0x0400003E)

/* Birim matrisin 8.8 sabit noktali 1.0 degeri. */
#define AFFINE_ONE  0x100

/* 0x080034EC */
void ResetBgScrollAndAffine(void)
{
    REG_BG0VOFS = 0;
    REG_BG0HOFS = 0;
    REG_BG1VOFS = 0;
    REG_BG1HOFS = 0;
    REG_BG2VOFS = 0;
    REG_BG2HOFS = 0;
    REG_BG3VOFS = 0;
    REG_BG3HOFS = 0;

    REG_BG2X_L = 0;
    REG_BG3X_L = 0;
    REG_BG2X_H = 0;
    REG_BG3X_H = 0;
    REG_BG2Y_L = 0;
    REG_BG3Y_L = 0;
    REG_BG2Y_H = 0;
    REG_BG3Y_H = 0;

    REG_BG2PA = AFFINE_ONE;
    REG_BG3PA = AFFINE_ONE;
    REG_BG2PB = 0;
    REG_BG3PB = 0;
    REG_BG2PC = 0;
    REG_BG3PC = 0;
    REG_BG2PD = AFFINE_ONE;
    REG_BG3PD = AFFINE_ONE;
}
