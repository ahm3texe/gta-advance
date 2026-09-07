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
/* 0x08030B50 -- 16 bayt -- BYTE-MATCHING
 *
 * Kayit sozunu bulup kip 2 ile iletiyor. src/world/lookup_then_call.c
 * (0x08030C0C) ile ayni kalip, tek fark ikinci argumanin sabit olmasi.
 *
 * KAYNAKTA ILK SIRADA OLMALI: dosyanin link tabani bu fonksiyonun
 * adresi (en kucuk ROM adresi); basa konmazsa `bl` ofsetleri kayar. */
void FUN_08030b50(u32 index)
{
    FUN_0802af40(GetTextString(index), 2);
}

/* ======================================================================= */
