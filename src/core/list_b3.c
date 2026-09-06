/* Alt nesneyi hucre kayitlarindan silip "bitti" isaretleme — 0x0800DC44
 * 112 bayt.  DURUM: 104/112 bayt ayni, 8 bayt fark (asagida).
 *
 * NE YAPIYOR
 *   0x02015650'deki hucre tablosunun "kirli hucre" listesini dolasiyor.
 *   Liste basi +0x00'deki sayac, girisleri +0x04'ten baslayan 4 baytlik
 *   (s16 x, s16 y) ciftleri.  Her giris 16x16'lik hucre izgarasinda bir
 *   hucreyi gosteriyor (hucre 12 bayt, satir 192 bayt).  Hucrenin
 *   +0x00'inda kayit sayisi, +0x08'inde Sub* dizisi var; dizide gelen
 *   nesneye esit olan her giris sifirlaniyor.  Sonda nesnenin +0x0C
 *   bayrak kelimesine 0x400 (SUB_DONE) ekleniyor.
 *
 * TIPLER NEREDEN
 *   Grid yerlesimi kardes src/core/listhead_e2.c'de olculmustu (hucre 12
 *   bayt = count/value/data, izgara +0x24, genislik/yukseklik +0xC24/+0xC28).
 *   Buradaki fark: +0x00 ile +0x04..+0x23 arasi orada "durum + dolgu"
 *   diye gecmisti; bu fonksiyon onlari SAYAC + (x,y) GIRIS DIZISI olarak
 *   kullaniyor (ldrsh -> isaretli s16, `bge`/`blt` -> isaretli sayac).
 *   +0x0C bayragi olan nesne src/core/nodelist_a4.c'deki `Sub`; 0x400
 *   maskesi orada SUB_DONE adiyla gecti, ayni ad korundu.
 *   0x02015650 data/ram_map.csv'de YOK; kardes dosyalar gibi ham adres
 *   cast'i ile geciliyor.  Sembol kaydi gerekiyor (rapora yazildi).
 *
 * OLCULEN YAZIM KALDIRACLARI (hepsi tek tek denendi)
 *
 * 1) `p` TEK BIR ISARETCI, IKI KEZ ATANIYOR.  ROM giris alanlarini
 *    `taban+6` / `taban+4` isaretcisi kurup i*4'u ONA ekleyerek okuyor
 *    (`adds r0,r6,#6 / adds r0,r1,r0 / movs r2,#0 / ldrsh`).  Duz
 *    `grid->touched[i].y` yazimi bunun yerine `taban+i*4` ortak alt
 *    ifadesini kurup 6/4'u INDIS yazmacina koyuyor -- 3 komut kisa.
 *    Ayri `py`/`px` yerelleri de olmuyor: agbcc ilkini dongu disina
 *    tasiyor (r8 push/pop, +16 bayt).  TEK yerel iki kez atanınca
 *    `set_in_loop != 1` oluyor ve ikisi de dongude kaliyor.
 *
 * 2) SATIR OFSETI AYRI DEYIMDE, BAYT CINSINDEN (`* ROWB`).  ROM once
 *    y*192'yi hesapliyor, SONRA x'i okuyup x*12 + taban yapiyor ve en
 *    sona topluyor.  `&cells[y][x]` yazimi tabani y terimine bagliyor
 *    (fold `(taban + y*192) + x*12` kanonikligini zorluyor) ve y carpimi
 *    x okumasindan SONRA cikiyor.  Ofseti ayri deyimde tutmak ROM'un
 *    sirasini veriyor.  `y * GW` (hucre cinsinden) olmuyor: iki ayri
 *    carpim uretiyor.
 *
 * 3) SON TOPLAMA TAMSAYI OLARAK (`off + (s32)&cells0[x]`).  Isaretci
 *    aritmetigi yazilirsa fold isaretciyi hep SOLA aliyor ve
 *    `adds r0,r0,r2` cikiyor; ROM'da `adds r0,r2,r0` var.
 *
 * 4) `j = i * 4` AYRI DEYIM.  ROM indisi isaretci tabanindan ONCE
 *    hesapliyor; `p[i * 2]` yazildiginda indis tabandan sonra geliyor.
 *
 * 5) `none` AYRI YEREL (kural 45).  ROM sifir sabitini `ldr r2,[r0,#8]`
 *    ONCESINDE kuruyor; kaynakta duz `0` yazilirsa agbcc sabiti ic
 *    dongunun on-blogunun SONUNA tasiyor (iki komut yer degistiriyor).
 *
 * 6) `gp` DONGU DISINDA, `grid` DONGU ICINDE (iki ayri yerel).
 *    `cells0` (izgara tabani = grid+36) gp'den turetilince agbcc
 *    `movs r0,#36 / adds r0,r0,r6` uretiyor (ROM boyle); tek yerelle
 *    `adds r1,#36` olup BIR KOMUT eksik kaliyor.  `grid` dongu icinde
 *    atandigi icin agbcc onu on-bloga tasiyip ROM'daki
 *    `adds r6,r0,#0` yazmac kopyasini uretiyor; dongu kosulundaki
 *    `gGrid->count` de ayni sekilde `adds r7,r1,#0` kopyasina donuyor.
 *
 * 7) `i = 0` DONGUDEN ONCE ayri deyim: ROM `movs r4,#0`u havuz
 *    yuklemesinden ONCE cikariyor.
 *
 * KALAN 8 BAYT -- YAZMAC DAGITIMI (r0/r1 takasi)
 *   ROM: `ldr r0,=0x02015650 / ldr r1,[r0]`, bizde tam tersi.  Bu iki
 *   sozde-yazmac kapi blogunda dogup on-blokta oluyor; dagitici sirayi
 *   floor_log2(refs)*refs/omur ile veriyor (kural 50) ve bizde sayac
 *   pseudo'su once geliyor.  tools/dump_alloc.py: havuz pseudo'su
 *   p24 refs 3 / omur 10 / oncelik 0.300, sayac pseudo'su oncelik 0.500.
 *   Callee-saved dagitim (r4=i, r5=sub, r6=grid, r7=n) ROM ILE AYNI --
 *   yani sorun yalnizca gecici yazmac secimi.
 *   Denenip elenenler: 11 bildirim donusu, ic dongunun uc bicimi
 *   (do/while, while, `--k`), ROWB'nin uc yazimi, `i` tipi int/s32,
 *   kosulun `gp->count` hali (124 bayt), gp/grid rollerinin takasi
 *   (97 fark), iki ayri dongu-ici kopya (93 fark).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/list_b3.c
 */

#include "gba_types.h"

/* Izgara satiri: 16 hucre x 12 bayt. */
#define GW              16
#define ROWB            (GW * 12)

/* Alt nesnenin +0x0C bayragi (src/core/nodelist_a4.c ile ayni ad). */
#define SUB_DONE        0x400

/* 0x02015650 data/ram_map.csv'de YOK; ham adres cast'i ile geciliyor. */
#define gGrid           ((Grid *)0x02015650)

/* Kayitlarda tutulan nesne; burada yalniz bayrak alani kullaniliyor. */
typedef struct Sub {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} Sub;

/* Izgara hucresi (yerlesim src/core/listhead_e2.c'de olculdu). */
typedef struct Cell {
    s32   count;                /* +0x00 kayit sayisi */
    u32   value;                /* +0x04 */
    Sub **data;                 /* +0x08 kayit dizisi */
} Cell;

/* Kirli hucre girisi: izgara koordinati. */
typedef struct Touched {
    s16 x;                      /* +0x00 sutun (12 bayt adim) */
    s16 y;                      /* +0x02 satir (192 bayt adim) */
} Touched;

typedef struct Grid {
    s32     count;              /* +0x0000 kirli hucre sayisi */
    Touched touched[8];         /* +0x0004 */
    Cell    cells[GW][GW];      /* +0x0024 */
    s32     width;              /* +0x0C24 */
    s32     height;             /* +0x0C28 */
} Grid;

/* 0x0800DC44 */
void FUN_0800dc44(Sub *sub)
{
    Grid *grid;
    Grid *gp;
    Cell *cells0;
    Cell *cell;
    Sub **entry;
    Sub  *none;
    s16  *p;
    s32   i;
    s32   j;
    s32   k;
    s32   off;

    i = 0;
    gp = gGrid;
    for (; i < gGrid->count; i++) {
        grid = gGrid;
        cells0 = gp->cells[0];

        /* Giris alanlari: taban+6 / taban+4 isaretcisine i*4 ekleniyor.
         * `p` BILEREK iki kez atandi (baslik notu 1). */
        j = i * 4;
        p = &grid->touched[0].y;
        off = *(s16 *)((u8 *)p + j) * ROWB;
        p = &grid->touched[0].x;
        cell = (Cell *)(off + (s32)&cells0[*(s16 *)((u8 *)p + j)]);

        k = cell->count;
        if (k > 0) {
            none = 0;
            entry = cell->data;
            do {
                if (*entry == sub)
                    *entry = none;
                entry++;
                k--;
            } while (k != 0);
        }
    }

    sub->flags |= SUB_DONE;
}
