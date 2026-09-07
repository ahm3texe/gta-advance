/* Iki katmanli varlik listesinde her dugum icin cizim gonderimi
 * 0x0801515C-0x080151BF, 100 bayt  [ESLESTI]
 *
 * ROM'un yaptigi is (kardes ReleaseEntryResources ile ayni gezinme iskeleti):
 *   0x020230A0'daki bas isaretcisinden baslayarak +0x3C ile bagli dis
 *   listeyi geziyor.  Her dis dugum ayni zamanda +0x44 ile bagli ic
 *   listenin BASI oluyor.  Ic listedeki her dugum icin, +0x30 alani
 *   doluysa:
 *     node->unk1c = AllocDrawEntry(node->unk10, (w * h) >> 1,
 *                                w >> 3, h >> 3, node->flags20);
 *     if (node->kind27 == 1) node->flags20 |= 0x2000;
 *   Burada w = +0x14, h = +0x15 bayt alanlari; piksel olcusu gibi
 *   davraniyorlar: 8'e bolununce karo sayisi, carpilip ikiye bolununce
 *   4bpp bayt boyutu cikiyor.  Kardes dosyadan farkli olarak +0x30
 *   burada ARALIK suzgecinden gecmiyor, sadece sifir mi diye bakiliyor.
 *
 * ROM'DAN OKUNAN AYRINTILAR
 * -------------------------
 * (1) +0x30 KONTROLU carpimdan SONRA geliyor: r6/r2/r3 yuklemeleri ve
 *     `muls`/`asrs` dalin ustunde duruyor.  Bu yuzden yuklemeler ve
 *     yarim-boyut hesabi kaynakta da `if`ten ONCE ayri deyimler.
 *     Olculdu: `half`i `if`in icine almak 20, `src`i icine almak 26
 *     bayt fark birakiyor -- agbcc yuklemeleri dalin ustune tasimiyor.
 * (2) W VE H YERELLERI u8 OLMALI -- ESLESMEYI ACAN TEK OLCUM.
 *     u32 yazildiginda komut akisi HARFI HARFINE ayni cikiyor, sadece
 *     r5 ile r6 yer degistiriyor (8 bayt): `outer` r6'ya, `src` r5'e
 *     dusuyor, ROM'da tersi.  Yani kural 50 kaldiraci burada dogrudan
 *     `outer`/`src` uzerinden degil, dar tipli iki komsu pseudo'nun
 *     omru uzerinden calisiyor; dar tip dagitim sirasini ceviriyor ve
 *     `outer` once dagitilip r5'i aliyor.  u8/u32 karisimi (biri dar,
 *     oteki genis) yine 8 bayt, s32 ise 10 bayt fark veriyor.
 * (3) Isaretlilik cakismasi gorunustedir: ayni r2/r3 icin ROM hem
 *     `asrs r1,r0,#1` (isaretli) hem `lsrs r2,r2,#3` (isaretsiz)
 *     kullaniyor.  u8 yerel int'e yukseldigi icin `(w * h) >> 1`
 *     isaretli kaydirma verirken, degerin ust bitleri sifir bilindigi
 *     icin `w >> 3` mantiksal kaydirmaya sadelestiriliyor.  Kaynakta
 *     zorlama gerekmiyor: `(s32)` donusumu ve `/ 2` yazimi da ayni
 *     baytlari veriyor, en yalin olan birakildi.
 * (4) Besinci arguman yigittan geciyor (`sub sp,#4` + `str r0,[sp,#0]`),
 *     ldrh ile okunup 32 bit yaziliyor: imza son parametreyi u16 aliyor
 *     (u32 yazmak da ayni baytlari veriyor, dar tip ROM'a daha sadik).
 * (5) +0x27 ofseti ldrb immediate sinirini (#31) astigi icin agbcc
 *     adresi kendiliginden ayri yazmaca aliyor; kaynakta isaretci
 *     yereli YOK (kardes dosyada da boyle olculmustu).
 * (6) Iki dongu de GIRIS KORUMALI + alttan donen bicim; ic dongunun
 *     korumasi dis degiskeni (r5) test ediyor, cunku `node = outer;`
 *     kopyasindan sonra kosul CSE ile outer uzerinden yaziliyor.
 *     Kural 49: fonksiyonun sonunda seyrek govde yok.
 *
 * DENENIP ELENEN YAZIMLAR
 * -----------------------
 * - `u32 w, h` (ve u8/u32 karisimi, `s32`): komut akisi ayni, r5<->r6
 *   ters, 8-10 bayt fark.  Yukarida (2).
 * - Kural 33 bicimi (`bits = 0x2000; bits |= flags20; flags20 = bits;`):
 *   ROM'daki sabit kopyasini (`adds r0,r1,#0`) SILIYOR, 96 bayt cikiyor.
 *   Bu fonksiyonda dogru yazim duz `|=`; kural 33 evrensel degil.
 * - `src`i `if`in icine almak (26 bayt), `half`i icine almak (20 bayt).
 * - Etkisiz kalanlar (yine 8 bayt, yani dagitimi cevirmiyorlar):
 *   bildirim sirasi permutasyonlari, `for` bicimli iki dongu, ic dongu
 *   icin acik `if (outer != 0)` korumasi, ayri `head` yereli (kural 22
 *   denemesi), `src`in `u8 *` yazilmasi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/level_step_b2.c
 */

#include "gba_types.h"

/* Gonderim yordami: kaynak, 4bpp bayt boyutu, karo olcusu ve bayraklar.
   Imza ROM'un cagri kurulumundan okundu; fonksiyonun kendisi
   (0x08012E78) henuz cozulmedi. */
void *AllocDrawEntry(void *src, s32 size, u32 tilesX, u32 tilesY, u16 flags);

typedef struct Entry {
    u8            pad00[0x10];
    void         *unk10;        /* +0x10 AllocDrawEntry'in ilk argumani */
    u8            width14;      /* +0x14 piksel genisligi */
    u8            height15;     /* +0x15 piksel yuksekligi */
    u8            pad16[6];
    void         *unk1c;        /* +0x1C cagrinin donusu buraya yaziliyor */
    u16           flags20;      /* +0x20 */
    u8            pad22[5];
    u8            kind27;       /* +0x27 */
    u8            pad28[8];
    u32           unk30;        /* +0x30 dolu olma kontrolu */
    u8            pad34[8];
    struct Entry *next3c;       /* +0x3C dis liste baglantisi */
    u8            pad40[4];
    struct Entry *next44;       /* +0x44 ic liste baglantisi */
} Entry;

/* +0x20'ye kurulan bayrak; anlami cozulmedi, deger ROM'dan alindi. */
#define FLAG_SUBMITTED 0x2000

/* 0x020230A0: dis listenin bas isaretcisi.  Kardes dosyadaki gerekce
   ayni: ofset 0 oldugu icin kural 1'in katlama sorunu olusmuyor, ROM da
   adresi havuzdan tek parca okuyup `ldr r5,[r0,#0]` yapiyor.  Bu adres
   icin data/ram_map.csv kaydi gerekiyor; sembol tanimlama yetkim yok. */
#define gListHead020230A0 (*(Entry **)0x020230A0)

/* 0x0801515C */
void LoadEntryTileData(void)
{
    Entry *outer;
    Entry *node;
    void *src;
    u8 w;                       /* dar tip zorunlu -- baslikta (2) */
    u8 h;
    s32 half;

    outer = gListHead020230A0;
    while (outer != 0) {
        node = outer;
        while (node != 0) {
            src = node->unk10;
            w = node->width14;
            h = node->height15;
            half = (w * h) >> 1;
            if (node->unk30 != 0) {
                w >>= 3;
                h >>= 3;
                node->unk1c = AllocDrawEntry(src, half, w, h, node->flags20);
                if (node->kind27 == 1) {
                    node->flags20 |= FLAG_SUBMITTED;
                }
            }
            node = node->next44;
        }
        outer = outer->next3c;
    }
}
