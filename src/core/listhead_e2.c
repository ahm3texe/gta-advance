/* Hucre tablosunu kurup arena paylastirma — 0x0800CB08-0x0800CC63 (348 bayt)
 *
 * PARK EDILDI: 291/348 bayt farkli (kalan fark tamamen REGISTER DAGITIMI;
 * komut dizisinin 103/173'u birebir ayni). Ayrintili eleme kaydi asagida.
 *
 * NE YAPIYOR
 *   1) DMA3 ile 0x0201AA80'e 3 kelime sifir (kucuk blok), REG_IME kaydet/
 *      geri-yukle ciftinin icinde.
 *   2) 0x02015650'deki tablonun +0xC24 / +0xC28 alanlarina gelen iki
 *      koordinatin `>> 10` kaydirilmisi yaziliyor (asrs = ISARETLI); bunlar
 *      asagidaki iki dongunun sinirlari. Dorduncu parametre (0/1/2) ROM'daki
 *      uc tablodan birini secip 0x0201AA98'e yaziyor; ucu de tek `str`
 *      paylasiyor (agbcc cross-jumping, kural 29'un tersi yonu).
 *   3) DMA3 ile 0x02016290'a 0x11F8 kelime sifir (buyuk blok). Bu blok
 *      ARENA olarak paylastiriliyor: her hucre kendi `count` degeri kadar
 *      kelime aliyor, `data` isaretcisi count*4 ilerliyor.
 *   4) Sonda tablo durum kelimesi, gListHead02016280 ve 0x0201AA9C bayragi
 *      sifirlaniyor.
 *
 * OLCULEN YERLESIM (disassembly'den)
 *   kaynak akisi (param0)  8 bayt:  +0x00 u16 count, +0x04 u32 value
 *   hucre                 12 bayt:  +0x00 count, +0x04 value, +0x08 arena
 *   tablo (0x02015650)             +0x00 durum, +0x24 16x16 hucre,
 *                                  +0xC24 width, +0xC28 height
 *   0x24 + 16*16*12 = 0xC24 tam oturuyor, ara dolgu yok.
 *   Dis dongu i (adim 12 bayt) width'e, ic dongu j (adim 192 bayt = 16*12)
 *   height'e karsi kosuyor; kaynak akisi ikisi boyunca KESINTISIZ ilerliyor.
 *
 * ADRESLEME: taban ROM'da DUZ yukleniyor, ofsetler AYRI literal olarak
 * kuruluyor (`ldr r0,=0x02015650 / ldr r4,=0xc24 / adds r3,r0,r4`).
 * Bunun extern sembol gerektirdigi sanilabilir; OLCULDU, gerektirmiyor:
 * agbcc sabit cast'ta da, ram_map'teki gercek bir extern'de de AYNI
 * bicimi uretiyor (probe: her iki bicim de `ldr taban / ldr ofset / add`).
 * Yani kural 1'in katlanma tuzagi burada olusmuyor. Bes adresin hicbiri
 * (0x02015650, 0x02016290, 0x0201AA80, 0x0201AA98, 0x0201AA9C)
 * data/ram_map.csv'de yok; kardes dosya src/core/list_b1.c ucunu ayni
 * bicimde yaziyor.
 *
 * OLCULEN DORT AYRINTI (her biri tek basina denendi)
 *
 * 1) TABAN AYRI YEREL OLMALI, ama ILK IKI YAZIM SABIT CAST'TAN gelmeli.
 *    Bastan `grid = gGrid02015650;` yazip her yerde `grid->` kullanmak
 *    tabanla ilk iki yazimi tek pseudo'ya baglayip `mov ip,taban` komutunu
 *    ONE aliyor; ROM'da kopya iki yazimdan SONRA (`mov r8,r0`). Iki yazimi
 *    sabit cast ile, sonrasini yerelle yazmak ROM'un sirasini veriyor
 *    (CSE ayni blok icinde tabani zaten tek pseudo'da birlestiriyor).
 *    Tamamen sabit cast ile yazmak ise tabani dongude DUSURUYOR: dongu
 *    ici erisimler `.word 0x2016274` gibi KATLANMIS literallere donuyor
 *    (olculdu: 308/352 fark, boyut da tutmuyor).
 *
 * 2) `gListHead02016280` HAM ADRES olarak yazildi, extern sembol olarak
 *    DEGIL -- bilincli ve OLCULMUS bir sapma. agbcc bir SYMBOL_REF'i
 *    donguden ONCE bir register'a kaldiriyor (`ldr r0,=sembol / mov r9,r0`)
 *    ve o register'i tum dongu boyunca tutuyor; ROM adresi EN SONDA
 *    yukluyor. Olcum: extern bicim 360 bayt / 320 fark, ham adres bicimi
 *    348 bayt / 291 fark. CONST_INT ayni sekilde kaldirilmiyor.
 *    (docs/COMPILER.md "kural 1 evrensel degildir" notu.)
 *    Ad yorumda birakildi ki yeniden adlandirmada bulunabilsin.
 *
 * 3) IC DONGU SINIRI: ROM'da kapi (guard) degeri dis dongu ONCESINDE BIR
 *    KEZ okunup `ip`'de tutuluyor, ama dongu ALTINDAKI kosul her turda
 *    bellekten YENIDEN OKUNUYOR (`ldr r0,[r6]`). Ayni ifadenin iki kopyasi
 *    icin bu ancak ALIAS KUMELERI farkliysa olur: `s32` okuma ile hucreye
 *    yapilan `u32` yazimlar ayni alias kumesinde oldugu icin agbcc kapiyi
 *    kaldiramiyor. Kapiyi `*(long *)` uzerinden okumak -- `long` ile `int`
 *    C'de AYRI alias kumeleri, isaretli/isaretsiz cifti gibi birlesmiyor --
 *    kapiyi kaldirilabilir yapiyor, dongu altindaki duz okuma ise
 *    kaldirilamaz kaliyor. Bu tam olarak ROM'un deseni.
 *    Olcum: `s32` kapi 352 bayt/313 fark, `long` gorunum 348 bayt/291 fark.
 *    Kural 39'un ("nitelemeyi tum alana degil O ERISIME uygula") alias
 *    kumesi hali.
 *
 * 4) Dis dongu `for`, ic dongu ACIK `do/while` (kural 40). Ic donguyu de
 *    `for` yazmak kapi ile alt kosulu tek ifadeye baglayip 3. maddedeki
 *    ayrimi imkansiz kiliyor (olculdu: 340 bayt, ROM'dan 8 bayt KISA).
 *
 * DENENIP ELENENLER (tekrar denenmesin)
 *   - Tum erisimler sabit cast (`gGrid02015650->...`): 352/308, dongu ici
 *     adresler katlaniyor, taban register'da tutulmuyor.
 *   - `Grid` yerine ayri nesneler (`(s32*)0x02016274` gibi): taban+ofset
 *     bicimi kayboluyor, elendi.
 *   - Extern sembol tabani (ram_map'teki gercek bir sembolle probe edildi):
 *     sabit cast ile BIREBIR ayni kod. Yani eksik sembol bir engel DEGIL.
 *   - `height` yerelde onbelleklenip hem kapida hem alt kosulda kullanmak:
 *     340 bayt (8 KISA), alt kosuldaki okuma kayboluyor.
 *   - Alt kosulu `volatile` gorunumden okumak: `loop_has_volatile` acilip
 *     adres kaldirmalarini da oldurdugu icin 340 bayt, daha kotu.
 *   - `long` yalniz struct alaninda / hem width hem height / yalniz kapida:
 *     ucu de ayni 291; en okunabiliri (yalniz kapida cast) secildi.
 *   - Kaynak sirasi permutasyonlari (`cells`/`src`/`width` atamalarinin
 *     sirasi): 291 / 291 / 295. En iyisi birakildi.
 *   - `cells` yereli olmadan (`&grid->cells[j][i]` dogrudan): 340 bayt,
 *     agbcc +0x24'u i*12 icine katliyor, ROM ise `grid+36`yi ayri
 *     dongu-degismezi olarak tutuyor.
 *
 * KALAN FARKIN KAYNAGI
 *   ILK FARK +0x00A (0x0800CB12): ROM `sub sp,#8`, bizimki `sub sp,#4` --
 *   yani daha en basta ROM'un BIR fazla yigin yuvasi var (width tasmasi).
 *   Sekil buyuk olcude oturdu; kalan fark DAGITIM. ROM: taban r8, hucre
 *   tabani r9, &height sl, height degeri ip, width YIGINDA (`str r3,[sp,#4]`,
 *   bu yuzden `sub sp,#8`). Bizimki: taban ip, hucre tabani r9, width sl,
 *   height r8, &height ic on-blokta yeniden hesaplaniyor, `sub sp,#4`.
 *   Yani ROM bizde olmayan BIR tasma (spill) daha yapiyor. tools/dump_alloc.py
 *   ile bakildi: taban pseudo'su 19 referans / 110 omur -> oncelik 0.691;
 *   yuksek register'lar arasinda EN SON dagitiliyor, ROM'da ise ILK.
 *   Onceligi cevirmek icin tabanin referans/omur oranini degistirmek
 *   gerekiyor; bu turda bulunamadi.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * Kural 3:  yigin gecicisi `volatile`, yoksa ikinci sifir yazimi eleniyor.
 * Kural 42/9: dongu sayaclari ISARETLI (`s32`) -- ROM `bge`/`blt` uretiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/listhead_e2.c
 */

#include "gba_io.h"

#define DMA_FILL_BIG    0x850011F8
#define DMA_FILL_SMALL  0x85000003

/* Hucre tablosunun satir genisligi: 192 bayt / 12 bayt = 16. */
#define GRID_WIDTH      16

/* Hicbiri data/ram_map.csv'de yok; ham adres cast'i ile geciliyor.
 * gListHead02016280 haritada VAR ama burada bilerek ham adres kullanildi
 * (baslik yorumu, madde 2). */
#define gGrid02015650    ((Grid *)0x02015650)
#define gArena02016290   ((u32 *)0x02016290)
#define gBuf0201AA80     ((void *)0x0201AA80)
#define gTable0201AA98   (*(const void **)0x0201AA98)
#define gListHead02016280 (*(void **)0x02016280)   /* ram_map: gListHead02016280 */
#define gFlag0201AA9C    (*(u8 *)0x0201AA9C)

/* Uc mod tablosu ROM'da; kural 1 yalniz RAM icin gecerli. */
#define TABLE_MODE_0    ((const void *)0x08EBFBA4)
#define TABLE_MODE_1    ((const void *)0x08EC14B4)
#define TABLE_MODE_2    ((const void *)0x08EC2DC4)

/* Kaynak akisinin girisi: 8 bayt, ikinci yarim-kelime kullanilmiyor. */
typedef struct Spec {
    u16 count;                  /* +0x00 */
    u16 pad02;
    u32 value;                  /* +0x04 */
} Spec;

typedef struct Cell {
    u32  count;                 /* +0x00 */
    u32  value;                 /* +0x04 */
    u32 *data;                  /* +0x08 arena dilimi */
} Cell;

typedef struct Grid {
    u32  status;                /* +0x00 */
    u8   pad04[0x20];
    Cell cells[GRID_WIDTH][GRID_WIDTH];   /* +0x24 */
    s32  width;                 /* +0xC24 */
    s32  height;                /* +0xC28 */
} Grid;

/* 0x0800CB08 */
void FUN_0800cb08(const Spec *spec, s32 x, s32 y, s32 mode)
{
    u16 ime;
    volatile u32 zero;
    Grid *grid;
    Cell *cells;
    Cell *cell;
    const Spec *src;
    u32 *data;
    s32 width;
    s32 i;
    s32 j;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = gBuf0201AA80;
    REG_DMA3.control = DMA_FILL_SMALL;
    (void)REG_DMA3.control;
    REG_IME = ime;

    /* Ilk iki yazim sabit cast'tan, taban yereli ONDAN SONRA: ROM'daki
     * `mov r8, r0` sirasi ancak boyle cikiyor (baslik yorumu, madde 1). */
    gGrid02015650->width = x >> 10;
    gGrid02015650->height = y >> 10;
    grid = gGrid02015650;

    if (mode == 0)
        gTable0201AA98 = TABLE_MODE_0;
    else if (mode == 1)
        gTable0201AA98 = TABLE_MODE_1;
    else if (mode == 2)
        gTable0201AA98 = TABLE_MODE_2;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    /* Arena isaretcisi DMA hedefiyle ayni yerelden geliyor: ROM `ldr r4`
     * ile yukledigi hedefi dongude paylastirici olarak kullanmayi
     * surduruyor. */
    data = gArena02016290;
    REG_DMA3.dst = data;
    REG_DMA3.control = DMA_FILL_BIG;
    (void)REG_DMA3.control;
    REG_IME = ime;

    src = spec;
    cells = grid->cells[0];
    width = grid->width;
    for (i = 0; i < width; i++) {
        j = 0;
        /* Kapi `long` gorunumden okunuyor: hucreye yapilan `u32` yazimlarla
         * ayri alias kumesinde oldugu icin agbcc bu okumayi dis dongunun
         * disina kaldiriyor. Asagidaki duz okuma kaldirilmiyor; ROM'un
         * deseni tam olarak bu (baslik yorumu, madde 3). */
        if (j < *(long *)&grid->height) {
            cell = cells + i;
            do {
                if (src->count == 0) {
                    cell->count = 0;
                    cell->value = 0;
                    cell->data = 0;
                } else {
                    /* Bellek ifadeleri BILEREK tekrarlandi: `bne` hedefi
                     * yeni bir genisletilmis blok basi oldugu icin agbcc
                     * `src->count` okumasini burada YENIDEN yayiyor.
                     * Yerele almak tek okuma uretip ROM'dan sapiyor. */
                    cell->count = src->count;
                    cell->value = src->value;
                    cell->data = data;
                }
                data += src->count;
                src++;
                cell += GRID_WIDTH;
                j++;
            } while (j < grid->height);
        }
    }

    grid->status = 0;
    gListHead02016280 = 0;
    gFlag0201AA9C = 0;
}
