/* Dugum havuzlarini sifirlayip serbest listelere dizme
 * 0x08054450-0x0805456F  (288 bayt)
 *
 * Fonksiyon iki asamadan olusuyor:
 *
 *   1) Bes ayri DMA3 doldurmasi. Dorttu 32-bit sifir dolgusu, sonuncusu
 *      16-bit 0x7FEF (bos kimlik) dolgusu. Her transfer REG_IME
 *      kaydedilip sifirlanarak, sonunda geri yuklenerek yapiliyor --
 *      src/core/init_sprite_pool.c ve src/world/slot_table.c ile ayni
 *      kalip. Kaynak yigindaki sabit bir hucre (DMA kontrolunde
 *      "kaynak sabit" biti kurulu).
 *
 *   2) Uc liste basligi List2Init ile bosaltiliyor, ardindan uc havuzun
 *      TUM girisleri kimligi 0x7FEF yapilip List2PushFront ile kendi
 *      listesinin basina takiliyor.
 *
 * Havuz olculeri DMA uzunluklariyla dongulerin adimlarindan cikti ve
 * birbirini dogruluyor:
 *
 *      0x02032B60  128 x 52 bayt = 6656 = 0x680 kelime  -> liste 0x02035A80
 *      0x02034560   64 x 72 bayt = 4608 = 0x480 kelime  -> gNodeListHead
 *      0x020357F0   16 x 40 bayt =  640 = 0x0A0 kelime  -> gRam02035780
 *      0x02030D50  gSlotArray, 128 x 60 = 7680 = 0x780 kelime
 *      0x02030C10  gSlotIds,   160 x u16, 0x7FEF ile dolduruluyor
 *
 * Uc havuz tabani ve 0x02035A80 liste basligi data/ram_map.csv'de YOK.
 * Bu dosya data/ altina yazmadigi icin onlar #define sabit cast olarak
 * duruyor; adlar GECICI. Sembolleri eklemek gerekiyor (bkz. gorev
 * ciktisindaki new_symbols).
 *
 * OLCULEN DORT AYRINTI (hepsi eslesmeyi tek basina degistirdi):
 *
 * 1) `zero` ve `fill` volatile: ROM her transferde ayni degeri yigina
 *    YENIDEN yaziyor (`str r5,[sp,#0]` dort kez). volatile olmadan agbcc
 *    ikinci ve sonraki yazmalari gereksiz gorup siliyor.
 *
 * 2) Ilk dongu ARTAN ISARETLI indisle (`i <= 127`) ve DIZI INDISIYLE
 *    (`entryA[i]`) yazildi. Boylece isaretci `i`nin bir turev induksiyon
 *    degiskeni (giv) oluyor; agbcc `i`yi tamamen eleyip son degeri
 *    `taban + 127*52` (0x19CC) olarak kuruyor ve ISARETLI `ble`
 *    uretiyor -- kural 42'nin devami. Elle yazilmis isaretci
 *    karsilastirmasi (`p <= end`) ISARETSIZ `bls` veriyordu.
 *
 * 3) Ayni dongude taban icin AYRI YEREL sart (kural 37/22). `POOL_A[i]`
 *    dogrudan yazilinca agbcc `i`yi eleyemeyip sayaci koruyor
 *    (`adds r4,#1 / cmp r4,#127`); `entryA = POOL_A;` ara yereliyle
 *    `entryA[i]` yazilinca ROM'un `cmp r6,r4 / ble` bicimi cikiyor.
 *
 * 4) Ikinci ve ucuncu dongu AZALAN sayacli ve ISARETCI YURUYUSLU: orada
 *    isaretci ayri bir temel induksiyon degiskeni oldugu icin sayac
 *    elenemiyor ve ROM'daki `subs`/`cmp`/`bge` cikiyor. Artirim sirasi
 *    ROM'daki gibi once isaretci sonra sayac (kural 43).
 *    Bos kimlik bu iki donguye AYRI DEYIMLE (`id = ID_NONE;`) veriliyor:
 *    dongu govdesindeki ciplak sabit, dongu-degismezi olarak preheader'in
 *    SONUNA tasiniyordu (taban, sayac, sabit); ROM sabiti EN BASTA
 *    istiyor (sabit, taban, sayac). Kaynak deyimi olarak yazmak sirayi
 *    duzeltiyor. Ilk donguye gerek yok: oradaki sabit 16-bit DMA
 *    dolgusundaki `fill = ID_NONE` ile ortak alt ifade olup r5'te duruyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_d4.c
 */

#include "gba_io.h"
#include "node_list.h"

#define ID_NONE      0x7FEF

/* DMA3 kontrol bitleri: etkin + kaynak sabit (+ 32 bit). */
#define DMA_FILL_32  0x85000000
#define DMA_FILL_16  0x81000000

/* gSlotIds: 160 u16 kimlik (src/world/slot_table.c ile ayni gorunum). */
#define SLOT_COUNT       160
/* gSlotArray: 128 x 60 baytlik yuva (src/world/slot_scan.c ile ayni). */
#define SLOT_ARRAY_LEN   128
#define SLOT_ARRAY_WORDS (SLOT_ARRAY_LEN * 15)

#define POOL_A_COUNT 128
#define POOL_A_WORDS (POOL_A_COUNT * 13)   /* 52 bayt / giris */
#define POOL_B_COUNT 64
#define POOL_B_WORDS (POOL_B_COUNT * 18)   /* 72 bayt / giris */
#define POOL_C_COUNT 16
#define POOL_C_WORDS (POOL_C_COUNT * 10)   /* 40 bayt / giris */

/* Cift bagli liste basligi ve dugumu: src/core/list_ops2.c ile ayni. */
typedef struct Node2 {
    struct Node2 *next;         /* +0x00 */
    struct Node2 *prev;         /* +0x04 */
} Node2;

typedef struct List2 {
    Node2 *head;                /* +0x00 */
    Node2 *tail;                /* +0x04 */
    int    count;               /* +0x08 */
} List2;

/* Uc havuz da ayni basligi tasiyor, yalniz adimlari farkli. */
typedef struct EntryA {
    struct EntryA *next;        /* +0x00 */
    struct EntryA *prev;        /* +0x04 */
    u16 id;                     /* +0x08 */
    u8  pad0A[42];              /* adim 52 */
} EntryA;

typedef struct EntryB {
    struct EntryB *next;        /* +0x00 */
    struct EntryB *prev;        /* +0x04 */
    u16 id;                     /* +0x08 */
    u8  pad0A[62];              /* adim 72 */
} EntryB;

typedef struct EntryC {
    struct EntryC *next;        /* +0x00 */
    struct EntryC *prev;        /* +0x04 */
    u16 id;                     /* +0x08 */
    u8  pad0A[30];              /* adim 40 */
} EntryC;

typedef struct Slot {
    u8  pad00[0x28];
    u32 mark;                   /* +0x28: 0 ise bos */
    u8  pad2C[0x10];            /* adim 60 */
} Slot;

/* ram_map.csv'de kayitli olanlar extern, olmayanlar sabit cast. */
extern u16  gSlotIds[SLOT_COUNT];       /* 0x02030C10 */
extern Slot gSlotArray[SLOT_ARRAY_LEN]; /* 0x02030D50 */
extern NodeC4 *gNodeListHead;           /* 0x02035A70 */

#define POOL_A ((EntryA *)0x02032B60)
#define POOL_B ((EntryB *)0x02034560)
#define POOL_C ((EntryC *)0x020357F0)
#define LIST_A ((List2 *)0x02035A80)

extern void List2Init(List2 *list);
extern void List2PushFront(List2 *list, Node2 *node);

/* 0x08054450 */
void FUN_08054450(void)
{
    EntryA *entryA;
    EntryB *entryB;
    EntryC *entryC;
    s32 i;
    s32 j;
    s32 k;
    u16 ime;
    u16 id;
    volatile u32 zero;
    volatile u16 fill;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = POOL_A;
    REG_DMA3.control = DMA_FILL_32 | POOL_A_WORDS;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = POOL_B;
    REG_DMA3.control = DMA_FILL_32 | POOL_B_WORDS;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = POOL_C;
    REG_DMA3.control = DMA_FILL_32 | POOL_C_WORDS;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = gSlotArray;
    REG_DMA3.control = DMA_FILL_32 | SLOT_ARRAY_WORDS;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = ID_NONE;
    REG_DMA3.src = (const void *)&fill;
    REG_DMA3.dst = gSlotIds;
    REG_DMA3.control = DMA_FILL_16 | SLOT_COUNT;
    REG_DMA3.control;
    REG_IME = ime;

    List2Init(LIST_A);
    List2Init((List2 *)&gNodeListHead);
    List2Init((List2 *)&gRam02035780);

    entryA = POOL_A;
    for (i = 0; i <= POOL_A_COUNT - 1; i++) {
        entryA[i].id = ID_NONE;
        List2PushFront(LIST_A, (Node2 *)&entryA[i]);
    }

    id = ID_NONE;
    entryB = POOL_B;
    for (j = POOL_B_COUNT - 1; j >= 0; entryB++, j--) {
        entryB->id = id;
        List2PushFront((List2 *)&gNodeListHead, (Node2 *)entryB);
    }

    id = ID_NONE;
    entryC = POOL_C;
    for (k = POOL_C_COUNT - 1; k >= 0; entryC++, k--) {
        entryC->id = id;
        List2PushFront((List2 *)&gRam02035780, (Node2 *)entryC);
    }
}
