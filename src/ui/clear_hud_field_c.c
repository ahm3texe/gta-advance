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
 * 0x08030F50 — BYTE-MATCHING.  ClearHudFieldB (0x08030F28) ile KOMUT
 * KOMUT ayni; yalniz iki VRAM sutunu farkli.  Bu yuzden ayni indisli
 * dongu yazimi kullanildi (hud_fields.c "F28: DIZI INDISI YAZIMI"):
 * karo sabiti dogrudan store ifadesinde durmali, yerele alinirsa SImode'a
 * yukselip ROM'un `adds r3,r0,#0` kopyasi kayboluyor.
 * ------------------------------------------------------------------ */
#define COL_C_LEFT   ((vu16 *)0x06009952)
#define COL_C_RIGHT  ((vu16 *)0x06009992)

void ClearHudFieldC(void)
{
    vu16 *a;
    vu16 *b;
    s32 i;

    a = COL_C_LEFT;
    b = COL_C_RIGHT;
    for (i = 0; i < 6; i++) {
        a[i] = TILE_BLANK;
        b[i] = TILE_BLANK;
    }
}

