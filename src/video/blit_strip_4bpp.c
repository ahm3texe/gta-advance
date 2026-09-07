/* HUD karo yazicilari + 4bpp serit ciziciler bandi — 0x08030B40 .. 0x08031A1C
 *
 * Ayni komsuluktaki eslesen dosyalar: src/ui/hud_fields.c (0x08030B60,
 * 0x08030E78, 0x08030F28), src/world/area_cleanup.c (0x08030CB4),
 * src/world/area_flags.c.  Bos karo sabiti 0xF0E8 ve 32 girisli (64 bayt)
 * karo haritasi satir adimi oradan geliyor.
 *
 * ---------------------------------------------------------------------
 * YERLESIM UYARISI (bu dosyanin iki fonksiyonu icin)
 * ---------------------------------------------------------------------
 * tools/agbcc_build.py bir C dosyasinin TUM fonksiyonlarini, dosyadaki
 * EN KUCUK adresten (burada 0x08030B40) baslayarak ARDISIK linkliyor.
 * ROM'da aralarinda baska fonksiyonlar oldugu icin ikinci ve sonraki
 * fonksiyonlar kendi ROM adreslerine DUSMUYOR.  Bu, `bl` iceren
 * fonksiyonlarda dal ofsetini bozuyor -- kaynak dogru olsa bile.
 * Etkilenenler: SendTextMode2 (3/12 bayt) ve DrawTwoDigits (dal ofseti).
 * Ikisi de KENDI adreslerinden derlenince BYTE-MATCHING; olculdu:
 *     tek fonksiyonluk dosya -> `python3 tools/verify_c_function.py <dosya>`
 *     SendTextMode2  12  BYTE-MATCHING  (0x080315C0)
 *     DrawTwoDigits 156  BYTE-MATCHING  (0x08031498)
 * `bl` icermeyen digerleri (0x08030F50, 0x080311DC, 0x08031328,
 * 0x08031A1C) konumdan bagimsiz oldugu icin bu dosyada da eslesiyor.
 * Ayni durum src/ui/hud_fields.c'de de var (752 ve 136 baytlik bosluklar)
 * -- orada hicbir fonksiyon cagri yapmadigi icin sorun cikmiyor.
 *
 * ---------------------------------------------------------------------
 * 0x0803173E — SINIR YANLIS, FONKSIYON DEGIL (olculdu, C yazilmadi)
 * ---------------------------------------------------------------------
 * Kanit:
 *  1. 0x0803173E'deki ilk komut `adds r4,#1`; prolog yok.
 *  2. 0x080317DC'deki `b.n 0x80316CE` GERIYE, kayitli baslangictan ONCEYE
 *     daliyor -- yani govde 0x0803173E'den once basliyor.
 *  3. 0x080317DE'deki epilog `pop {r3,r4,r5} / mov r8..sl / pop {r4-r7} /
 *     pop {r0} / bx r0`; buna karsilik gelen prolog 0x080316B0'da:
 *     `push {r4,r5,r6,r7,lr} / mov r7,sl / mov r6,r9 / mov r5,r8 /
 *      push {r5,r6,r7} / sub sp,#4`.
 *  4. 0x08031684-0x080316AF arasi AYRI ve tam bir fonksiyon
 *     (`push {r4,lr}` ... `bx r0`, 0x080316AC'de havuz kelimesi
 *     0x02025810) -- data/functions.csv'de hic kayitli degil.
 * Sonuc: 186 baytlik "bosluk" aslinda iki fonksiyon; gercek sinir
 * 0x080316B0-0x080317EE (318 bayt) ve 0x0803173E onun govde ortasi.
 * 0x0803173E icin C yazilmadi.  (Gercek fonksiyon 0x08031A1C'nin
 * kardesi: ayni sekiz-nibble maskeli serit cizici, tek fark satir
 * sayisi ve parametre yerlesimi.)
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/band_c.c
 */

#include "gba_io.h"
#include "gba_types.h"

#define TILE_BLANK   0xF0E8

extern u32  GetTextString(u32 index);
extern void FUN_0802af40(u32 text, u32 kind);

/* --------------------------------------------------------------------
 * 0x08031A1C — BYTE-MATCHING, 470 bayt.  4bpp (nibble basina bir piksel)
 * serit cizici.
 *
 * 48 satir; her satirda sekiz nibble okunup iki yarim kelimeye
 * paketleniyor (dst satir basina 4 bayt ilerliyor).  Satirin dikey
 * kirpma araligi disinda kalmasi (row < 0 ya da row >= rowLimit) HIZLI
 * yolu seciyor: sekiz nibble dogrudan kaynaktan.  Aralik icinde ise her
 * nibble ayri ayri deneniyor; sutun sayaci (col) negatifken alttaki
 * goruntuyle harmanlaniyor: (*mask & *under) | *src.
 * Isaretci adimlari: src ve under satir basina 48 bayt, mask ise
 * 8 + maskStep.
 *
 * OLCULEN (fark 157 -> 0 yolu):
 *  1. DONGU AZALAN yazilmali.  ROM `movs #48 / ... / add r9,-1 / cmp #0 /
 *     beq` uretiyor ve satiri `(rowBase + 48) - sayac` diye YENIDEN
 *     hesapliyor (yigindan `rowBase`i her turda okuyarak).  `for (i = 0;
 *     i < 48; i++)` -- s32/u32, for/do-while farketmez -- agbcc'de ARTAN
 *     kaliyor (`add r0,r9` + `cmp #0x2f`), kural 42 burada gecmiyor.
 *     `row = rowBase + (48 - i)` yazimi ROM'un uc komutunu birebir
 *     veriyor; `(rowBase + 48) - i` ve `rowBase + 48 - i` yeniden
 *     birlesip `rowBase - (i - 48)` oluyor (82/238).
 *  2. Kirpma testi TEK `if (a || b)` olmali.  Bir sinir degisken
 *     oldugu icin kural 60'taki katlama olmuyor; agbcc `bge HIZLI /
 *     bge MASKELI / (dusus) HIZLI` uretiyor -- ROM'un blok sirasi bu.
 *  3. `(*mask & *under)` sirasi ONEMLI: agbcc AND'in IKINCI operandini
 *     once yukluyor (kural 53'un `&` karsiligi).  `(*under & *mask)`
 *     yazilirsa `ldrb [r5]` one geciyor (84 -> 87 komut farki).
 *  4. HIZLI YOLDA mask/under NIBBLE BASINA artirilmali.  ROM'da tek bir
 *     `adds r5,#8 / adds r4,#8` gorunuyor ve ilk refleks onu kaynaga
 *     oyle yazmak; o zaman n0..n3 gecicilerinin omurleri kayiyor ve
 *     yazmac oncelikleri KIL PAYI ters donuyor (col r7 / n2 r6 olarak
 *     dagitiliyor, ROM'da col r6 / n2 r7).  Sekiz ayri `mask++/under++`
 *     yazilinca dagitim ROM'unki oluyor; artirimlari birlestirip tek
 *     `+= 8` yapmayi derleyici KENDISI, dagitimdan SONRA yapiyor.
 *     Olculen oncelikler (tools/dump_alloc.py):
 *         yanlis yazim:  n2 1.383 (r6) > col 1.358 (r7)
 *         dogru yazim:   col 1.358 (r6) > n2 1.258 (r7)
 *     ELENEN ara yazimlar: `mask += 8` blok basi/ortasi/sonu (87..197),
 *       `mask += 4` yarim kelime basina (196..233), gecicileri dallara
 *       gore bolmek (78..80), gecicilerin tipi/bildirim sirasi (etkisiz),
 *       `n2++; n2--;` omur uzatan no-op (kural 50) -- BU DA TAM ESLESME
 *       veriyor ama savunulabilir kaynak degil, o yuzden alinmadi.
 * ------------------------------------------------------------------ */
#define STRIP_ROWS   48
#define ROW_BYTES    40

void BlitStrip4bpp(s32 rowBase, s32 colBase, s32 rowLimit, s32 maskStep,
                  const u8 *mask, const u8 *under, u16 *dst, const u8 *src)
{
    s32 i;
    s32 row;
    s32 col;
    u32 n0;
    u32 n1;
    u32 n2;
    u32 n3;

    for (i = STRIP_ROWS; i != 0; i--) {
        col = colBase;
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
            if (col++ >= 0) {
                n0 = *src;
                src++;
                mask++;
                under++;
            } else {
                n0 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ >= 0) {
                n1 = *src;
                src++;
                mask++;
                under++;
            } else {
                n1 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ >= 0) {
                n2 = *src;
                src++;
                mask++;
                under++;
            } else {
                n2 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ >= 0) {
                n3 = *src;
                src++;
                mask++;
                under++;
            } else {
                n3 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
            if (col++ >= 0) {
                n0 = *src;
                src++;
                mask++;
                under++;
            } else {
                n0 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ >= 0) {
                n1 = *src;
                src++;
                mask++;
                under++;
            } else {
                n1 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ >= 0) {
                n2 = *src;
                src++;
                mask++;
                under++;
            } else {
                n2 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ >= 0) {
                n3 = *src;
                src++;
                mask++;
                under++;
            } else {
                n3 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
        }
        mask += maskStep;
        under += ROW_BYTES;
        src += ROW_BYTES;
    }
}
