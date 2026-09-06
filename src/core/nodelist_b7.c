/* Hedefe dogru kaydirilmis konumu dene, tutmazsa komsu karolari tara
 * 0x08055054-0x0805518B  (312 bayt; son 4 bayt literal havuzu)
 *
 * Cagriya bir konum isaretcisi (in/out) ve bir (dx, dy) kaymasi geliyor.
 * Once `konum + (dx,dy)<<16` noktasi FUN_08054f1c ile snaniyor; bos ise
 * konum oraya tasinip 1 donuluyor. Degilse istenen nokta cevresinde
 * +-128.0 (0x800000) genisliginde bir kutu kurulup FUN_08040700'e
 * veriliyor; o da kutuya giren en fazla 8 karonun (tx, ty) indekslerini
 * yaziyor. Her karo icin karo merkezi (tx<<22 + dx<<16 + 32.0) tekrar
 * snaniyor; ilk bos karo bulundugunda konumun x/y'si oraya cekilip 1
 * donuluyor. Hicbiri tutmazsa 0.
 *
 * Tarama en fazla IKI tur: ilk turda (yalniz useTileMask ise) oyuncunun
 * uzerinde durdugu karo turunun maskesi (1 << GetTileFieldA2), ikinci
 * turda cagricinin verdigi maske kullaniliyor. Iki maske ayni ciktiysa
 * ya da useTileMask sifirsa tek tur yetiyor.
 *
 * YAPI IPUCLARI:
 *   - Konum 12 baytlik {s32 x, y, z}: ilk basarida `ldmia/stmia {r2,r3,r4}`
 *     ile tek seferde kopyalaniyor (kural 32 -- struct atamasi).
 *   - Kutu iki ayri Vec3: FUN_08040700 onu [r0,#0]/[r0,#4] ve
 *     [r0,#12]/[r0,#16] diye okuyor, yani 12 bayt adimli 2'lik dizi.
 *   - Karo tamponu 8 x {u16 tx, u16 ty} = 32 bayt (sp+40..71).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_b7.c
 *
 * OLCULEN DORT AYRINTI (her biri tek basina denendi, dordu de belirleyici):
 *
 * 1) KAYMA SABITI (dx<<16)+32.0 IC DONGU GOVDESINDE YEREL OLMALI.
 *    ROM onu ic dongunun on-basligina (guard'dan SONRA) tasimis: yani
 *    kaynakta ic dongunun ICINDE yaziliyor ve loop-invariant motion
 *    disari cikariyor. Ic donguden ONCE (dis dongu govdesinde) yazmak
 *    agbcc'ye onu DIS dongudan da cikartip iki fazla yigin yuvasina
 *    (0x68/0x6c) yaymasi icin izin veriyor: cerceve 108 -> 116, 308 bayt.
 *    Ifadeyi satir ici yazmak ise hic tasimiyor: fold `x + (y + SABIT)`
 *    ifadesini `(x + SABIT) + y` haline getirip dx yuklemesini dongunun
 *    icinde birakiyor (300 bayt).
 *
 * 2) IKI ALAN IKI FARKLI ADRESLEME ISTIYOR. ROM tx'i yuruyen isaretciyle
 *    (`ldrh r2,[r4,#0]` + `adds r4,#4`), ty'yi indeksle okuyor
 *    (`lsls r1,r6,#2` + `mov r0,sp` + `adds r0,#42`). Ikisini de
 *    `cells[i].tx` / `cells[i].ty` yazmak agbcc'nin combine_givs'ini
 *    tetikliyor: tek isaretci + `[r4,#2]` cikiyor (6 bayt eksik, 304).
 *    Ikisini de isaretciyle yazmak (`cell->tx`, `cell->ty`) ise IKI ayri
 *    yuruyen isaretci uretiyor (308). Cozum: tx alan erisimi kalir,
 *    ty ise `((u16 *)cells)[i * 2 + 1]` diye AYRI bir goruntuden okunur
 *    -- bu bicim giv olarak birlestirilmiyor ve ROM'un indeks hesabini
 *    aynen veriyor. 304 -> 312 bayt.
 *
 * 3) `fallback` YERELI `tileMask`TEN ONCE BILDIRILMELI. Ikisi de yigina
 *    dokuluyor ve reload yuvalari PSEUDO NUMARASI sirasina gore veriyor;
 *    pseudo numarasi da BILDIRIM sirasindan geliyor (expand_decl). ROM'da
 *    fallback 0x5c, tileMask 0x60. Ters bildirimde yuvalar takas oluyordu.
 *    Ayni mekanizmayla `yoff` bildirimi `count`tan SONRAYA alininca
 *    ybase r7 yerine r3'e, count r3 yerine r7'ye oturdu (19 -> 9 fark).
 *    9 bildirim adinin 138 permutasyonu tarandi; bu siralama en iyisi.
 *
 * 4) `pp = &probe;` IC DONGUNUN ILK DEYIMI OLMALI. ROM'un on-basliginda
 *    `add r5,sp,#72` EN BASTA duruyor; yani &probe kaynakta dongunun
 *    basinda bir yerele aliniyor ve movable olarak ilk siradan disari
 *    tasiniyor. Adresi yalnizca cagri argumaninda kullanmak (`&probe`)
 *    onu movable listesinin SONUNA koyuyor ve on-baslik sirasi bozuluyor.
 *
 * DIGER UYGULANAN KURALLAR:
 *   - Kural 9/31: `attempt`, `count`, `i` isaretli `int`; ROM `bge`/`blt`
 *     ve `ble` (isaretli) uretiyor.
 *   - Kural 32: ilk basarida `*pos = want;` struct atamasi ->
 *     `ldmia/stmia {r2,r3,r4}`.
 *   - Kural 35: son `pop {r1}; bx r1` ve r0'in canli olmasi -> donus u32.
 *   - Parametreler s32/u32; dar tip olsa girise lsls/lsrs normalizasyonu
 *     eklenirdi (kural 15), ROM'da yok.
 *
 * ELENEN YOLLAR (TEKRAR DENEME):
 *   - `cells[i].ty` (giv birlesmesi, 304) ve `cell->ty` (ikinci isaretci,
 *     308); `*(u16 *)((u8 *)cells + i*4 + 2)` (308); `volatile` ty (308).
 *   - xoff/yoff'u ic dongu ONUNDE hesaplamak (308) veya satir ici yazmak
 *     (300).
 *   - `for (i = 0, cell = cells; ...; i++, cell++)` bicimi (308).
 *   - Bildirim sirasi: 138 permutasyon tarandi, mevcut sira en iyisi.
 *   - Son 2 baytlik fark (`adds r3,r4,#0`) icin denenenler, HICBIRI
 *     tutmadi (hepsi >= 9 fark): ic ice `if`, `if/else`, ucul operator,
 *     ters kosul (`sel = tileMask` + `||`), `goto have_sel` etiketli
 *     bicim, `sel` tipini int/s32/u32 yapmak, `arg = sel;` ikinci yerel
 *     (kural 17), `count` tipini u32 yapmak, `MAX_CELLS`/`box`/`cells`
 *     argumanlarini ayri yerele almak (kural 18), `&box[0]` yazimi,
 *     `fallback` yerine dogrudan `mask` parametresi (308), `fallback`i
 *     fonksiyon basinda atamak (308), `fallback = mask`i dongu icine
 *     almak (20), 5. arguman icin `zero` yereli (13), `probeMask`i
 *     yerele almak (14), `sel`i donguden once atayip icinde guncellemek
 *     (carry: 316/34 ve 312/9), cagriyi if/else'te IKI KEZ yazmak (320),
 *     `u32 *selp` isaretcisiyle secmek (320), dip testte `sel == fallback`
 *     kullanmak (46). Derleyici varyanti `agbcc` de denendi: ayni sonuc.
 *
 * KALAN FARK -- MEKANIZMASI OLCULDU, KALDIRACI YOK
 *   tools/dump_alloc.py: `sel` pseudo'su (p31, 6 ref / 12 omur, oncelik
 *   1.000, dagitim sirasi 3, calls_crossed = 0) r3'e dusuyor. agbcc'nin
 *   global.c'sinde `find_reg`, cagri asmayan bir allocno icin r0..r3'u
 *   de aday sayar; r0/r1/r2 arguman kurulumuyla catistigi icin ilk bos
 *   olan r3 seciliyor. ROM'da ayni deger r4'te, yani orada r0..r3 aday
 *   DEGIL -- bu yalnizca `calls_crossed != 0` oldugunda olur (o zaman
 *   `call_used_reg_set` disari atiliyor ve ilk bos callee-saved r4 kaliyor).
 *   Yani ROM'un kaynaginda `sel`in omru FUN_08040700 cagrisini ASIYOR.
 *   Cagridan sonra ic donguye kadar ROM'da baska komut yok; omru uzatan
 *   her semantik-esdeger yazim (yukaridaki carry/guard/selp denemeleri)
 *   `sel`i ic dongudeki r4 (karo isaretcisi) ile de catistirip dokuyor.
 *   Kaynak duzeyinde baska bir kaldirac bulunamadi.
 *
 * DURUM: 312/312 bayt boyut TAM; 142/151 komut ayni; yazmac operand
 * sayimlarindan yalnizca r3 (22 vs 23), r4 (18 vs 15) ve r0 (74 vs 76)
 * sapiyor -- ucu de ayni tek farktan (eksik `adds r3,r4,#0` ve onun
 * yerine gelen 2 baytlik hizalama dolgusu) turuyor. Kalan 8 komut farki
 * bu 2 baytin kaydirdigi dal ofsetleridir, ayri bir hata degildir.
 */

#include "gba_types.h"

#define BOX_RADIUS      0x00800000      /* 128.0, 16.16 sabit nokta */
#define HALF_TILE       0x00200000      /*  32.0 = karo yarisi      */
#define PROBE_LIMIT     0x00180000      /*  24.0; FUN_08054f1c esigi */
#define MAX_CELLS       8
#define TILE_SHIFT      22              /* karo indeksi -> dunya kord. */
#define POS_SHIFT       16              /* tam sayi -> 16.16          */

/* Dunya konumu; z alani tasiniyor ama snamada kullanilmiyor. */
typedef struct Vec3 {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
    s32 z;                      /* +0x08 */
} Vec3;

/* FUN_08040700'un yazdigi karo indeksi cifti. */
typedef struct Cell {
    u16 tx;                     /* +0x00 */
    u16 ty;                     /* +0x02 */
} Cell;

extern u32 GetTileFieldA2(const Vec3 *pos);
extern u32 FUN_08054f1c(const Vec3 *pos, u32 mask, u32 limit);
extern int FUN_08040700(const Vec3 *box, Cell *out, int max, u32 mask, u32 opt);

/* 0x08055054 */
u32 FUN_08055054(Vec3 *pos, u32 mask, u32 useTileMask, u32 probeMask,
                 s32 dx, s32 dy)
{
    Vec3 want;
    Vec3 box[2];
    Cell cells[MAX_CELLS];
    Vec3 probe;
    Vec3 *pp;
    u32  fallback;
    u32  tileMask;
    u32  sel;
    s32  xoff;
    int  attempt;
    int  count;
    s32  yoff;
    int  i;

    want.x = pos->x + (dx << POS_SHIFT);
    want.y = pos->y + (dy << POS_SHIFT);
    want.z = pos->z;
    tileMask = 1 << GetTileFieldA2(pos);

    if (useTileMask != 0) {
        if (FUN_08054f1c(&want, probeMask, PROBE_LIMIT)) {
            *pos = want;
            return 1;
        }
    }

    box[0].x = want.x - BOX_RADIUS;
    box[1].x = want.x + BOX_RADIUS;
    box[0].y = want.y - BOX_RADIUS;
    box[1].y = want.y + BOX_RADIUS;

    fallback = mask;
    for (attempt = 0; attempt <= 1; attempt++) {
        sel = fallback;
        if (useTileMask != 0 && attempt == 0)
            sel = tileMask;

        count = FUN_08040700(box, cells, MAX_CELLS, sel, 0);
        for (i = 0; i < count; i++) {
            pp = &probe;
            xoff = (dx << POS_SHIFT) + HALF_TILE;
            pp->x = (cells[i].tx << TILE_SHIFT) + xoff;
            yoff = (dy << POS_SHIFT) + HALF_TILE;
            pp->y = (((u16 *)cells)[i * 2 + 1] << TILE_SHIFT) + yoff;
            pp->z = 0;
            if (FUN_08054f1c(pp, probeMask, PROBE_LIMIT)) {
                pos->x = pp->x;
                pos->y = pp->y;
                return 1;
            }
        }

        if (useTileMask == 0)
            break;
        if (tileMask == fallback)
            break;
    }

    return 0;
}
