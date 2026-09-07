/* band_a.c'den AYRILDI: proje bolge modeli dosya basina TEK BITISIK
 * ROM araligi istiyor. Bu fonksiyonlar ROM'da dagitik oldugu icin ayni
 * dosyada tutulamazlar (add_c_region "bolge eslesmiyor" der). Ayrica
 * agbcc_build.py cevirim birimini min(adres)e linkledigi icin dis cagri
 * iceren fonksiyonlar yanlis adreste kalirdi.
 */

/* Band A -- 0x080308EC .. 0x08031BF4 arasindan dokuz kucuk fonksiyon.
 *
 * DOSYA ICINDE ALTI FONKSIYON VAR, DOKUZ DEGIL. Uc tanesi data/ram_map.csv'de
 * bulunmayan sembolleri, biri de gRam02025810 blogunu istiyor; gerekcesi
 * asagida her birinin yerinde yaziyor. Eksik sembollu bir govde yazmak
 * agbcc_build.py'yi sys.exit ettirip DOSYANIN TAMAMINI dogrulanamaz
 * yapardi, o yuzden yalnizca not birakildi.
 *
 * ---------------------------------------------------------------------
 * TEK DOSYA / DAGINIK ADRESLER: `bl` OFSETLERI HAKKINDA
 * ---------------------------------------------------------------------
 * agbcc_build.py ceviri biriminin TAMAMINI, iceride tanimli fonksiyonlarin
 * EN KUCUK ROM adresine linkliyor (`base = min(...)`) ve fonksiyonlari
 * kaynak sirasinda arka arkaya diziyor. Bu dosyadaki fonksiyonlar ROM'da
 * bitisik DEGIL (aralarinda baska fonksiyonlar var), dolayisiyla
 * ilkinden sonrakiler yanlis adrese dusuyor. Fonksiyon govdesi ROM ile
 * birebir olsa bile, iceride `bl` varsa bagil ofset kayiyor ve
 * `make c-match` o fonksiyonu "farkli" gosteriyor.
 *
 * Bu yuzden `bl` iceren iki fonksiyon TEK BASINA da olculdu (gecici bir
 * dosyada, kendi ROM adresine linklenerek):
 *     FUN_080315f8  -> tek basina BYTE-MATCHING (30/30)
 *     FUN_08031bf4  -> tek basina BYTE-MATCHING (28/28)
 * Bu dosyada ikisi de yalnizca 4'er bayt fark gosteriyor ve farkli olan
 * baytlarin hepsi `bl` komutlarinin bagil ofset alanlari; komut dizisi,
 * yazmac dagitimi ve sabitler ROM ile ayni (diff_function.py ile
 * dogrulandi). FUN_08030b50 en kucuk adresli fonksiyon oldugu icin
 * kaynakta BASA konuldu; boylece o base'e dusuyor ve `bl`leri dogru
 * kodlaniyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/band_a.c
 */

#include "gba_types.h"


/* --- 0x020307F0 hucre izgarasi (src/core/mark_area_cells.c ile ayni) ---- */
typedef struct CellGrid {
    s32 originY;                /* 0x00 */
    s32 originX;                /* 0x04 */
    u8  cells[1];               /* 0x08 -- satir adimi 0x10 */
} CellGrid;

extern CellGrid gRam020307F0;

/* --- gRam02025810 +0x4C'deki 24 x 180 baytlik yuva dizisinin ogesi
 *     (src/world/release_slot.c) ---------------------------------------- */
typedef struct Held {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} Held;

/* Sabit noktali konum: her iki eksen de 22 bit kaydirilarak hucreye
 * cevriliyor (bkz. FUN_08031534). */
typedef struct Position {
    s32 x;                      /* 0x00 */
    s32 y;                      /* 0x04 */
} Position;

extern u32  GetTextString(u32 index);
extern void FUN_0802af40(u32 word, u32 kind);
extern s32  FUN_080309a8(Held *held);
extern u32  ReleaseSlot(u32 index);
extern u8  *FUN_0806de10(u8 *dest, const u8 *src, u32 size);

/* ======================================================================= */
/* 0x080317F0 -- 84 bayt -- BYTE-MATCHING
 *
 * 8 bit/piksel bir kaynagi 4 bit/piksel karo satirlarina paketliyor: her
 * turda kaynaktan sekiz bayt okuyup iki yarim soz yaziyor, sonra kaynagi
 * 40 bayt ilerletiyor (satir adimi 48, 48 tur, hedefe 192 bayt).
 *
 * IKI OLCULEN AYRINTI:
 *  - ROM'da sekizinci `*src++` artirimi GORUNMUYOR, onun yerine sondaki
 *    `adds r4,#41` var. Sebep: kaynaktaki sekizinci `src++` ile
 *    `src += 40` birlesiyor. Yani stride 48 = 8 okuma + 40 atlama;
 *    kaynakta asimetrik bir yazim (yedi `*src++` + bir `*src`) YOK.
 *  - Yazmac dagitimi ancak IKI AYRI GECICI KUMESI ile ROM'unki oluyor
 *    (kural 54). Tek kume (p0..p3'u iki blokta da kullanmak) src'yi
 *    r2'ye, biriktiriciyi r1'e dusuruyordu: 31/42 komut farkli.
 *    Ayrica denenip elenen: `for (i = 0; i < 48; i++)` bicimi (35 bayt
 *    fark; artan sayac maskeleme ve ters dongu testi uretiyor).
 *
 * Ilk bes parametre kullanilmiyor; kaynak ve hedef isaretcileri yigindan
 * (sp+20 / sp+24) okundugu icin imzada YEDI parametre olmak zorunda. */
void FUN_080317f0(u32 a, u32 b, u32 c, u32 d, u32 e, u16 *dst, const u8 *src)
{
    s32 i;
    u32 p0, p1, p2, p3;
    u32 q0, q1, q2, q3;

    i = 48;
    do {
        p0 = *src++;
        p1 = *src++;
        p2 = *src++;
        p3 = *src++;
        *dst++ = p0 | (p1 << 4) | (p2 << 8) | (p3 << 12);

        q0 = *src++;
        q1 = *src++;
        q2 = *src++;
        q3 = *src++;
        *dst++ = q0 | (q1 << 4) | (q2 << 8) | (q3 << 12);

        src += 40;
        i--;
    } while (i != 0);
}


/* ======================================================================= */
/* YAZILMAYAN DORT FONKSIYON -- gerekcesi ve ROM'dan cikarilan yapisi
 * ======================================================================= */

