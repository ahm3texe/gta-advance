/* Maskesiz/maskeli serit cizici ve yuva alt nesnesi birakma
 * — 0x08031684-0x080317ED
 *
 * SINIR NOTU: data/functions.csv burada uzun sure tek bir 0x0803173E
 * kaydi tutuyordu. O adres bir fonksiyon degil; 0x0803073C'deki sozde
 * `bl 0x0803173E` aslinda bir havuz kelimesi (0xF000FFFF) ve
 * discover_functions.py havuzu kod sanmisti. Olculen gercek yerlesim:
 * 0x08031684 (44 bayt) ve 0x080316B0 (318 bayt). Ikisi BITISIK oldugu
 * icin ayni ceviri biriminde durabiliyorlar.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/video/blit_strip_plain.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

/* ---- 0x08031684 — yuvanin alt nesnesini birakir ---------------------
 *
 * gRam02025810'daki 24 x 180 baytlik yuva dizisi (bkz.
 * docs/GRAM02025810_YERLESIM.md ve src/world/release_slot.c). Eleman ici
 * +0xB1 bayragi kuruluysa eleman ici +0x64'teki alt nesne birakilip
 * bayrak sifirlaniyor.
 *
 * Iki taban AYRI YERELDE kurulmali (release_slot.c ile ayni olcut):
 * bayrak (taban + olcek) + 0xED, alt nesne ise olcek + (taban + 0xA0)
 * seklinde toplaniyor -- ROM'un iki farkli birlesim sirasi bu.
 * ------------------------------------------------------------------ */

#define SLOT_STRIDE  180            /* 0xB4 */
#define OFF_SUB      0xA0           /* blok basi; eleman ici +0x64 */
#define OFF_FLAG     0xED           /* blok basi; eleman ici +0xB1 */

extern void FUN_08013abc(u8 *sub);

/* 0x08031684 */
void ReleaseSlotSub(u32 index)
{
    u8 *base;
    u8 *flag;
    u8 *subBase;
    u32 scaled;

    base = gRam02025810;
    scaled = index * SLOT_STRIDE;
    flag = base + scaled + OFF_FLAG;
    if (*flag == 0)
        return;

    subBase = base + OFF_SUB;
    FUN_08013abc(subBase + scaled);
    *flag = 0;
}

/* ---- 0x080316B0 — 318 bayt, 8x48 4bpp serit cizici ------------------
 *
 * src/video/blit_strip_4bpp.c'deki 0x08031A1C ile ayni ailedendir; fark:
 * BURADA SUTUN KIRPMASI YOK. Satir dikey aralik disindaysa (row >=
 * rowLimit ya da row < 0) sekiz kaynak nibble'i oldugu gibi aliniyor;
 * aralik icindeyse hepsi `(*mask & *under) | *src` ile harmanlaniyor.
 * Sekiz deger dorder bitlik alanlara paketlenip iki yarim soz olarak
 * dst'ye yaziliyor.
 *
 * Kardesten devralinan ve BURADA DA GECERLI olan uc olcum:
 *  1. Dongu AZALAN yazilmali ve satir `rowBase + (48 - i)` diye YENIDEN
 *     hesaplanmali; ROM `movs #48 / add ip,-1 / cmp #0 / beq` uretiyor.
 *  2. Kirpma testi TEK `if (a || b)` olmali; agbcc `bge HIZLI /
 *     bge MASKELI / (dusus) HIZLI` uretiyor, ROM'un blok sirasi bu.
 *  3. HIZLI YOLDA mask/under NIBBLE BASINA artirilmali. ROM'da tek bir
 *     `adds r6,#8 / adds r5,#8` gorunur ama kaynakta oyle yazilirsa
 *     gecicilerin omurleri kayiyor; birlestirmeyi derleyici dagitimdan
 *     SONRA kendisi yapiyor.
 * Satir sonu adimlari: mask degisken (maskStep), under ve src 40 bayt
 * (sekiz nibble zaten teker teker ilerledigi icin satir adimi 48).
 * ------------------------------------------------------------------ */

#define STRIP_ROWS   48
#define ROW_BYTES    40

/* 0x080316B0 */
void BlitStripPlain4bpp(s32 rowBase, s32 rowLimit, s32 maskStep,
                        const u8 *mask, const u8 *under, u16 *dst,
                        const u8 *src)
{
    s32 i;
    s32 row;
    u32 n0;
    u32 n1;
    u32 n2;
    u32 n3;

    for (i = STRIP_ROWS; i != 0; i--) {
        row = rowBase + (STRIP_ROWS - i);
        if (row >= rowLimit || row < 0) {
            n0 = *src++;
            mask++;
            under++;
            n1 = *src++;
            mask++;
            under++;
            n2 = *src++;
            mask++;
            under++;
            n3 = *src++;
            mask++;
            under++;
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
            n0 = *src++;
            mask++;
            under++;
            n1 = *src++;
            mask++;
            under++;
            n2 = *src++;
            mask++;
            under++;
            n3 = *src++;
            mask++;
            under++;
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
        } else {
            n0 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            n1 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            n2 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            n3 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
            n0 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            n1 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            n2 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            n3 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
        }
        mask += maskStep;
        under += ROW_BYTES;
        src += ROW_BYTES;
    }
}
