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
/* 0x08031534 -- 68 bayt -- BYTE-MATCHING
 *
 * Sabit noktali bir konumu hucre izgarasina cevirip o hucrenin BOS olup
 * olmadigini donduruyor. Izgara gorunumu src/core/mark_area_cells.c ile
 * ayni (originY +0, originX +4, cells +8, satir adimi 0x10).
 *
 * ROM'DAN OKUNAN UC AYRINTI:
 *  - Sutun korumasi TEK isaretsiz test (`cmp #31 / bhi`): kural 60'a gore
 *    bu `if (column < 0 || column > 31)`in katlanmis hali, yani kaynakta
 *    `column` isaretsiz ve tek karsilastirma.
 *  - Satir korumasi IKI ayri isaretli test (`cmp #0 / blt`, `cmp #31 /
 *    bgt`): iki ayri `if`. Ayni satirda `&&` ile birlestirmek katlardi.
 *  - Sonuc bir yerelde maddelesiyor (`movs r4,#0` daha adres
 *    hesabindan ONCE, sonra `movs r4,#1`): kural 48.
 *  - `return 0` govdesi havuzun ARDINDA, fonksiyonun sonunda: kural 49,
 *    yani erken `return 0` degil sona `goto`. */
u32 FUN_08031534(Position *pos)
{
    u32 column;
    s32 row;
    u32 result;

    column = (pos->x >> 22) - gRam020307F0.originX;
    row = (pos->y >> 22) - gRam020307F0.originY;
    if (column >= 0x20)
        goto none;
    if (row < 0)
        goto none;
    if (row >= 0x20)
        goto none;

    result = 0;
    if (gRam020307F0.cells[row * 0x10 + column] == 0)
        result = 1;
    return result;

none:
    return 0;
}


/* ======================================================================= */
