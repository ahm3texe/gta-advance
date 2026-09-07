/* Dogru parcasi ile dikdortgen kenarlarinin kesisimi -- 0x08064724-0x0806493B
 *
 * DURUM: ESLESMEDI. 508/536 bayt uretiyor; govde ve akis ROM ile ayni,
 * fark TEK BIR DAGITIM KARARINDA (asagida "KALAN FARK" bolumu). Dosya
 * calisir ve derlenir durumda birakildi ki bir sonraki oturum sifirdan
 * baslamasin.
 *
 * ---------------------------------------------------------------------
 * FONKSIYONUN NE YAPTIGI (hepsi ROM'dan okundu, uydurma yok)
 *
 * Egimi 16.16 sabit noktali `slope`, kaymasi `offset` olan bir DOGRUYU bir
 * dikdortgenin dort kenariyla kesistirir; kesisim gercekten kenar parcasi
 * uzerindeyse aday sayilir; adaylar arasindan `from` noktasina EN YAKIN
 * olanini `hit`e yazar ve o uzakligi dondurur. Hicbir kenar kesilmiyorsa 0.
 *
 *   steep == 0 -> dogru  y = (slope*x >> 16) + offset  biciminde;
 *                 kenar x'leri BOLMEYLE, kenar y'leri CARPMAYLA bulunur.
 *   steep != 0 -> dogru  x = (slope*y >> 16) + offset  biciminde; roller
 *                 tam simetrik olarak yer degistirir.
 *
 * Bunu ROM soyle belli ediyor: iki dalda da AYNI dort alan okunuyor ama
 * bolme/carpma ikilisi yer degistiriyor (0x806474a-0x806478c'ye karsi
 * 0x8064798-0x80647da), ve dort bayrak testinin ikisi x-sinirlarina
 * (alan 0 / alan 12) ikisi y-sinirlarina (alan 4 / alan 16) bakiyor.
 * Yani alan 0/12 bir eksenin, 4/16 otekinin sinirlari: sol/sag ve
 * ust/alt. Aday noktalar (xTop,top) (xBottom,bottom) (left,yLeft)
 * (right,yRight) -- dikdortgen kenarlariyla kesisim noktalari.
 *
 * ALAN 8 KULLANILMIYOR: adi verilmedi, `pad08` olarak birakildi.
 *
 * CAGRILANLAR (ikisi de ROM'da coz umlendi, bu dosyada sadece extern):
 *   __divsi3(pay, bolen) : isaretli bolme. Imza src/save/
 *       init_save_system.c'de zaten bu bicimde bildirilmis, aynen alindi.
 *       `%` ve `/` YAZILAMAZ -- agbcc __divsi3 uretir, sembol yok.
 *   FUN_0800c4bc(kareToplami) : 0x0800c4bc'de tablo tabanli karekok
 *       (0x0834289c'deki tabloyu 0xffff/0xffffff/... esiklerine gore
 *       oteleyerek okuyor). Cagri yeri hep dx*dx+dy*dy aliyor ve sonuc
 *       0x7fffffff'e karsi isaretli karsilastiriliyor -> s32 uzaklik.
 *
 * ---------------------------------------------------------------------
 * OLCULEN YAZIM KARARLARI (her biri ayri ayri derlenip ROM'a karsi tartildi)
 *
 * 1. `left` ve `top` AYRI YERELLER OLMALI (kural 45). Bayrak testleri
 *    box->left / box->top yerine yerelleri okumazsa:
 *        yereller yok                        -> 484 bayt, ONSOZ BILE TUTMUYOR
 *        yereller var (bu dosya)             -> 508 bayt, ONSOZ 9 komut TAM
 *    Fark kozmetik degil: yereller olmadan agbcc `from` ve `hit`
 *    parametrelerini YAZMAÇTA tutuyor; ROM ikisini de giriste yigina
 *    yaziyor (`str r1,[sp,#0]` / `str r2,[sp,#4]`). Yereller dusuk
 *    yazmac baskisini yukseltip iki parametreyi de ROM'daki gibi
 *    tasirtiyor. dump_alloc ile dogrulandi: SPILL kumesi artik
 *    {from, hit, yLeft, yRight} -- ROM'un dort yuvasiyla ayni SAYIDA.
 *
 * 2. `left` DAL B'NIN ICINDE, `top` DAL SONRASINDA atanir. Yerlesim
 *    onemli, cunku `left` bolme cagrilarini ASIYOR (callee-saved yazmac
 *    gerekiyor, ROM'da r4), `top` ASMIYOR (caller-saved yeter, ROM'da r3):
 *        ikisi de dal icinde (cagrilardan once)  -> 512 bayt, 5 yigin yuvasi
 *        ikisi de dal sonrasinda                 -> 480 bayt (en kotu)
 *        left dal icinde / top sonrasinda        -> 508 bayt, 4 yuva  <-- ROM
 *    `top`u da cagrilardan once atarsak fazladan bir callee-saved yazmac
 *    yaniyor ve yigin bes yuvaya cikiyor; ROM dort yuva kullaniyor.
 *
 * 3. Dal B'nin `slope == 0` kolunda `left = box->left;` acikca yazilir.
 *    ROM o kolda 0x80647c4'te `ldr r4,[r0,#0]` ile alani gercekten
 *    okuyor; satiri kaldirinca agbcc yuklemeyi birlesme noktasina
 *    tasiyip yazmac omrunu kisaltiyor ve 1. maddedeki kazanc kayboluyor.
 *
 * 4. `%` yasak; bolme dogrudan __divsi3 cagrisi olarak yazilir.
 *
 * 5. Bayraklar tek bir `s32 flags` uzerinde `|=` ile birikir; ROM
 *    0x80647ea-0x8064830 arasinda dort kez oku/or/yaz yapiyor.
 *
 * FARK YARATMAYAN (olculdu, hepsi 508'de kaldi -- bu satirlarda serbestsiniz):
 *   - `const` niteleyicileri (from/box): 0 bayt fark.
 *   - `flags` tipini u32 yapmak: 0 bayt fark.
 *   - dx/dy/dist icin her blokta AYRI yereller: 0 bayt fark (bloklar
 *     zaten ayri sozde-yazmac uretiyor).
 *   - `sum = dx*dx + dy*dy;` ara yereli: 0 bayt fark.
 *   - Yerel bildirim sirasini degistirmek: 0 bayt fark.
 *
 * ---------------------------------------------------------------------
 * KALAN FARK -- 28 bayt, TEK bir dagitim karari (bir sonraki oturum buradan
 * devam etsin, yukaridakileri TEKRAR DENEMESIN)
 *
 * ROM'un dagitimi                     benim ciktim
 *   r4 = left / best                    r4 = slope / best
 *   r5 = slope                          r5 = left
 *   r6 = xTop                           r6 = box      <-- DUSUK yazmac
 *   r7 = xBottom                        r7 = offset   <-- DUSUK yazmac
 *   r8 = box      <-- YUKSEK yazmac     r8 = flags
 *   r9 = yLeft    <-- YUKSEK yazmac     r9 = xTop
 *   sl = offset   <-- YUKSEK yazmac     sl = xBottom
 *   sp+0/4/8/12 = from,hit,yRight,flags sp+0/4/8/12 = from,hit,yLeft,yRight
 *
 * 28 baytin TAMAMI bundan geliyor: ROM `box`u r8'de, `offset`i sl'de
 * tuttugu icin her alan erisiminden once bir `mov rX,r8` / `mov rX,sl`
 * ekliyor (ROM'da 17 + 3 = 20 fazladan `mov`). Benim ciktim ikisini de
 * dusuk yazmaca koydugu icin `ldr r0,[r6,#12]` diyip o movlari atliyor.
 * Yani ROM DAHA KOTU kod uretmis; hedef, agbcc'yi ayni darliga sokmak.
 *
 * Kural 50 ile olculen oncelikler (dump_alloc.py --rom ciktisi):
 *      slope .569 | left .341 | box .254 | offset .221 | flags .200
 *      xTop .189  | xBottom .165 | from .163 | hit .152 | yLeft .119
 * Dort dusuk callee-saved yazmac (r4..r7) oncelik sirasina gore
 * slope/left/box/offset'e gidiyor. ROM'da ise xTop ve xBottom `box` ve
 * `offset`in ONUNDE; yani ROM'da box ve offset'in onceligi xBottom'un
 * (.165) ALTINA dusmus olmali. Aranacak kaldirac: box'un ref sayisini
 * dusuren ya da omrunu uzatan bir yazim. Denenmis ve ISE YARAMAYAN
 * yollar yukaridaki "FARK YARATMAYAN" listesinde.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/text_f5.c  -> 508/536, ESLESMEDI
 */

#include "gba_types.h"

/* Duzlemdeki nokta: her iki parametre de (kaynak ve sonuc) bu bicimde. */
typedef struct Point {
    s32 x;
    s32 y;
} Point;

/* Kesisim aranan dikdortgen. Alan 8 ROM'da hic okunmuyor; anlami
 * bilinmedigi icin ad verilmedi. */
typedef struct Bounds {
    s32 left;
    s32 top;
    s32 pad08;
    s32 right;
    s32 bottom;
} Bounds;

#define BOUND_TOP_HIT     1              /* ust kenarla kesisim gecerli */
#define BOUND_BOTTOM_HIT  2
#define BOUND_LEFT_HIT    4
#define BOUND_RIGHT_HIT   8
#define DIST_MAX          0x7fffffff

/* 0x0806C0F4 -- isaretli bolme (agbcc'nin __divsi3'u degil, oyunun kendi
 * yordami). Imza src/save/init_save_system.c'deki bildirimle aynidir. */
extern s32 __divsi3(s32 dividend, s32 divisor);

/* 0x0800C4BC -- tablo tabanli karekok; kare toplamini uzakliga cevirir. */
extern s32 FUN_0800c4bc(s32 squareSum);

/* 0x08064724 */
s32 FUN_08064724(s32 steep, const Point *from, Point *hit,
                 s32 slope, s32 offset, const Bounds *box)
{
    s32 xTop, xBottom, yLeft, yRight;
    s32 flags;
    s32 best, dist, dx, dy;
    s32 left, top;

    flags = 0;

    if (steep == 0) {
        /* y = (slope*x >> 16) + offset : ust/alt kenarlarin x'i bolmeyle. */
        if (slope != 0) {
            xTop    = __divsi3((box->top - offset) << 16, slope);
            xBottom = __divsi3((box->bottom - offset) << 16, slope);
        } else {
            xBottom = 0;
            xTop = 0;
        }
        top = box->top;
        left = box->left;
        yLeft  = ((slope * left) >> 16) + offset;
        yRight = ((slope * box->right) >> 16) + offset;
    } else {
        /* x = (slope*y >> 16) + offset : roller tam simetrik yer degistirir. */
        if (slope != 0) {
            left = box->left;
            yLeft  = __divsi3((left - offset) << 16, slope);
            yRight = __divsi3((box->right - offset) << 16, slope);
        } else {
            yRight = 0;
            yLeft = 0;
            left = box->left;
        }
        top = box->top;
        xTop    = ((slope * top) >> 16) + offset;
        xBottom = ((slope * box->bottom) >> 16) + offset;
    }

    /* Kesisim gercekten KENAR PARCASI uzerinde mi? Sinirlar disarida
     * birakilir (ROM ble/bge kullaniyor, yani kesin buyuk / kesin kucuk). */
    if (xTop > left && xTop < box->right)
        flags |= BOUND_TOP_HIT;
    if (xBottom > left && xBottom < box->right)
        flags |= BOUND_BOTTOM_HIT;
    if (yLeft > top && yLeft < box->bottom)
        flags |= BOUND_LEFT_HIT;
    if (yRight > top && yRight < box->bottom)
        flags |= BOUND_RIGHT_HIT;

    if (flags == 0)
        return 0;

    best = DIST_MAX;

    if (flags & BOUND_TOP_HIT) {
        dx = from->x - xTop;
        dy = from->y - top;
        dist = FUN_0800c4bc(dx * dx + dy * dy);
        if (dist < best) {
            best = dist;
            hit->x = xTop;
            hit->y = box->top;
        }
    }
    if (flags & BOUND_BOTTOM_HIT) {
        dx = from->x - xBottom;
        dy = from->y - box->bottom;
        dist = FUN_0800c4bc(dx * dx + dy * dy);
        if (dist < best) {
            best = dist;
            hit->x = xBottom;
            hit->y = box->bottom;
        }
    }
    if (flags & BOUND_LEFT_HIT) {
        dx = from->x - box->left;
        dy = from->y - yLeft;
        dist = FUN_0800c4bc(dx * dx + dy * dy);
        if (dist < best) {
            best = dist;
            hit->x = box->left;
            hit->y = yLeft;
        }
    }
    if (flags & BOUND_RIGHT_HIT) {
        dx = from->x - box->right;
        dy = from->y - yRight;
        dist = FUN_0800c4bc(dx * dx + dy * dy);
        if (dist < best) {
            best = dist;
            hit->x = box->right;
            hit->y = yRight;
        }
    }

    return best;
}
