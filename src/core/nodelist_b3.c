/* Varligin bagli dugumunu bosaltip baglanti bayraklarini yeniden kurma
 * 0x08053F6C, 136 bayt (son 4 bayt + orta 8 bayt literal havuzu).
 *
 * Varligin +0x2C alanindaki dugum alinip:
 *   1) alt nesnesi (+0x28) sifirlaniyor,
 *   2) +0x0B baytindaki "kirli" biti ve seviye yarim-bayti sifirlaniyor,
 *   3) dugumun +0x16 baglanti kimligi 0 ve 0x3FF disindaysa o kimlikle
 *      0x02035A80 listesinde bir dugum aranip, bulunanin +0x18 bayraginda
 *      0x1000 varsa ayni bit bizim dugume de yaziliyor,
 *   4) ayni kimlik ICIN ARAMA TEKRAR yapilip: bizim dugumde ya da
 *      bulunan dugumde 0x1000 varsa +0x18 bayrak kelimesi 0x1C03FFFF ile,
 *      yoksa 0x1C00FFFF ile maskeleniyor.
 *
 * ROM'DAN OLCULEN AYRINTILAR
 * --------------------------
 * - `pop {r4}; pop {r0}; bx r0` -> donus tipi void (kural 35).
 * - Prolog `push {r4,lr}`: cagrilari asan TEK bir callee-saved deger var
 *   (dugum isaretcisi, r4). Yani baglanti kimligi cagriyi ASMIYOR --
 *   ikinci aramanin argumani bellekten YENIDEN okunuyor
 *   (`ldrh r0,[r4,#22]`).
 * - Dugum yerlesimi kardes nodelist_a4.c / nodelist_b1.c'deki Node ile
 *   ayni ailede: +0x0B bayrak bayti, +0x18 bayrak kelimesi, +0x28 alt
 *   nesne. Bu dosyada ilk kez +0x16 u16 baglanti kimligi goruluyor
 *   (`ldrh`, yani u16).
 * - +0x18 alani BURADA isaretsiz: yalniz `& 0x1000` ve iki maske var,
 *   b1.c'deki `bits < 0` kapisi yok, o yuzden `u32` birakildi.
 * - 0x1000 sabiti havuzdan degil `movs #0x80 / lsls #5` ile kuruluyor ve
 *   AYNI yazmac hemen ardindan `orrs` icin tekrar kullaniliyor; yani
 *   `node->bits |= LINK_BIT` bicimi dogru (ayri maske yereli GEREKMIYOR).
 * - 0x3FF, 0x1C00FFFF ve 0x1C03FFFF havuz sabiti; 0x3FF u16 ile
 *   karsilastiriliyor (immediate'e sigmiyor). Kural 44 GEREKMIYOR:
 *   kanoniklestirme yalniz `<`/`<=` sinamalarinda oluyor, burada `==` var.
 *
 * +0x0B'DEKI TEK STRB'NIN SEBEBI (olculdu)
 * ----------------------------------------
 * ROM tek `ldrb` + iki `ands` (-2 ve 15) + tek `strb` uretiyor. Bu, arka
 * arkaya iki AYRI bitfield atamasinin (`dirty = 0;` sonra `level = 0;`)
 * sonucudur: ikinci atamanin `ldrb`si CSE ile, birincinin `strb`si de
 * olu-saklama elemesiyle dusuyor. Maske sirasi kaynak sirasini veriyor:
 * once -2 (dirty), sonra 15 (level).
 * Ayrica `movs r0,#0 / str r0,[r4,#40] / subs r0,#2` dizisi -2 sabitini
 * bir onceki sifirdan turetiyor; yani `node->sub = 0;` satiri bitfield
 * atamalarindan ONCE geliyor.
 *
 * DENEYIP ELEDIGIM YAZIMLAR (hepsi olculdu)
 * -----------------------------------------
 * 1. TEK ortak `other` yereli (iki aramanin sonucu ayni degiskene):
 *    136 bayt ama fark 68. Birinci aramanin sonucu r0'da kalamiyor,
 *    `adds r1,r0,#0` kopyasi cikiyor ve kuyruktaki r1/r2 rolleri de takas
 *    oluyor. Kural 45: her aramaya kendi yereli (`linked` / `other`)
 *    verilince fark 0. ROM'un birinci sonucu r0'da tuketip ikinciyi r2'ye
 *    koymasi tam olarak bu ayrimin izi.
 * 2. Ikinci aramaya yerel `id`i vermek (`FUN_08055954(id)`): fark 8.
 *    Deger cagriyi asiyor, prolog `push {r4,r5,lr}` oluyor ve kimlik r5'e
 *    dagitiliyor; ROM'da r5 YOK. Bu, "ikinci okuma bellekten" iddiasinin
 *    dogrudan kaniti -- kural 11'in tersi yonu.
 * 3. `level = 0;` once, `dirty = 0;` sonra: fark 90. Maskeler kaynak
 *    sirasini izliyor, ilk sabit `movs #15` oluyor ve -2 artik sifirdan
 *    turetilemeyip `movs #2 / negs` ile kuruluyor (bir komut fazla, tum
 *    havuz ofsetleri kayiyor).
 * 4. +0x0B'yi duz `u8 kind` yapip tek satirda `kind &= 0x0E`: 132 bayt,
 *    4 KISA. agbcc iki maskeyi katlayip `movs r0,#14` uretiyor; ROM'un
 *    iki ayri `ands`i ancak iki ayri bitfield atamasiyla cikiyor.
 *
 * ESLESME: 136/136 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_b3.c
 */

#include "gba_types.h"

#define LINK_NONE   0x3FF        /* +0x16 icin "baglanti yok" kimligi */
#define LINK_BIT    0x1000       /* +0x18 bayrak kelimesindeki baglanti biti */
#define KEEP_PLAIN  0x1C00FFFF   /* baglanti yokken korunan bitler */
#define KEEP_LINKED 0x1C03FFFF   /* baglanti varken ek olarak 0x30000 korunur */

/* 0x02035A80 listesindeki dugum; yerlesim src/core/nodelist_a4.c ve
 * src/core/nodelist_b1.c ile ayni aileden, +0x16 burada eklendi. */
typedef struct Node {
    struct Node *next;          /* +0x00 */
    u8    pad04[7];
    u8    dirty : 1;            /* +0x0B bit 0    */
    u8    pad0B : 3;            /* +0x0B bit 1..3 */
    s8    level : 4;            /* +0x0B bit 4..7 */
    u8    pad0C[10];
    u16   linkId;               /* +0x16 */
    u32   bits;                 /* +0x18 */
    u8    pad1C[12];
    void *sub;                  /* +0x28 */
} Node;

/* Cagiranlar (0x0803795C ve 0x0803846C) bu nesneyi r0'da veriyor; burada
 * yalniz +0x2C alani kullaniliyor, gerisi adlandirilmadi. */
typedef struct Entity {
    u8    pad00[0x2C];
    Node *node;                 /* +0x2C */
} Entity;

extern Node *FUN_08055954(u32 id);      /* 0x08055954: listede kimlik arar */

/* 0x08053F6C */
void FUN_08053f6c(Entity *ent)
{
    Node *node;
    Node *linked;
    Node *other;
    u32   flags;
    u32   id;

    node = ent->node;
    if (node == 0)
        return;

    node->sub = 0;
    node->dirty = 0;
    node->level = 0;

    id = node->linkId;
    if (id != 0 && id != LINK_NONE) {
        linked = FUN_08055954(id);
        if (linked != 0 && (linked->bits & LINK_BIT))
            node->bits |= LINK_BIT;
    }

    /* Ikinci arama argumanini bellekten yeniden okuyor (yukaridaki prolog
       notu); yerel `id` burada KULLANILMIYOR. */
    other = FUN_08055954(node->linkId);
    flags = node->bits;
    if ((flags & LINK_BIT) == 0
     && (other == 0 || (other->bits & LINK_BIT) == 0))
        node->bits = flags & KEEP_PLAIN;
    else
        node->bits &= KEEP_LINKED;
}
