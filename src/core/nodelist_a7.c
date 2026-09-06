/* Listede bir yuvayi arayip "secili dugum" olarak kurma - 0x08053DF8, 164 bayt.
 *
 * 0x02035A80 listesi bastan gezilip yuvasi (dugum +0x28) verilen hedefe esit
 * olan ilk dugum aranir. Nobetci kimlik 0x7FEF'e ya da liste sonuna once
 * varilirsa 0 donulur; bulunursa dugum baglama kaydinin +0x30 alanina yazilip
 * 1 donulur.
 *
 * ROM'dan OLCULEN ayrintilar
 * --------------------------
 * - Ilk parametre (r0) HIC kullanilmiyor: 0x08053DFC'de r0 hemen havuz
 *   sabitiyle eziliyor. Yine de imzada duruyor, cunku hedef r1'de ve kip
 *   r2'de geliyor. `s32 unused` adi bu yuzden.
 * - `pop {r4,r5}; pop {r1}; bx r1` -> donus DEGERI var (r0 canli), yani
 *   kural 35'in tersi: donus tipi void DEGIL. u32 secildi.
 * - Dugum yerlesimi: +0x00 next, +0x08 u16 kimlik, +0x18 bayrak kelimesi
 *   (0x400 biti test ediliyor), +0x28 yuva isaretcisi.
 * - Yuva yerlesimi: +0x0C bayrak kelimesi (0x10, 0x80, 0x01, 0x40, 0x100).
 * - Baglama kaydi: +0x28 yuva, +0x30 secili dugum.
 * - 0x02026F34 ve 0x020272C8 birer ISARETCI tutuyor; ROM once adresi r3'e
 *   koyup `ldr r0,[r3,#0]` ile iceriden okuyor. ram_map'te ikisi de `u32`
 *   olarak bildirilmis (src/world/node_search.c) -- CELISKILI extern tur
 *   yaratmamak icin burada tur degistirilmedi, adres alinip cast edildi.
 *
 * Dongu bicimi: `for (n = bas; n; n = n->next) { ... break; ... }` yazimi
 * ROM'un dondurulmus halini (giris korumasi + `b` ile kosula atlama, artirim
 * dongunun basinda) DOGRUDAN uretti. Kardes 0x08053D48 (nodelist_c4.c) ayni
 * aileden olmasina ragmen FARKLI bir bicim kullaniyor (nobetciyi r5'e
 * kopyalayip dongu icinde tasiyor); kopyalanmadi, ROM'dan okundu.
 *
 * DENENIP ELENEN YAZIMLAR
 * -----------------------
 * 1. `Ctx **base` (volatile'siz): 160 bayt, fark 14. agbcc `(*base)`i bir kez
 *    okuyup r3'te tutuyor; ROM ise +0x28 ve +0x30 erisimleri icin AYRI AYRI
 *    `ldr r0,[r3,#0]` yapiyor. Cozum kural 39'un dar hali: isaretcinin
 *    kendisini `Ctx *volatile *` yapmak. Alani (Ctx govdesini) volatile
 *    yapmak degil -- yalniz bu erisim.  160 -> 164 bayt, fark 69 -> 10.
 * 2. `if (kind == 2) base = A; else base = B;` (tek yerel): agbcc B'yi
 *    KARSILASTIRMADAN ONCE yukleyip A'yi kosullu uzerine yaziyor, aradaki
 *    `b` kayboluyor. `b` kaybolunca ondan sonra gelen havuz dokumu de
 *    kayboluyor -- ROM'daki 0x08053E58'deki inline `.word 0x02026F34` tam
 *    olarak o `b`nin arkasindaki dokum. Fark 10'da takildi.
 * 3. `base = (kind == 2) ? A : B;` ucluk: 2 ile birebir ayni kod, fark 10.
 * 4. Acik `goto` ile iki blok (kural 40) TEK BASINA yetmedi: CFG ayni oldugu
 *    icin agbcc yine hoist etti, fark 10.
 * 5. KURAL 45 cozdu: her dala KENDI yereli (`primary` / `secondary`) verilip
 *    ortak `base`e oradan atanmasi bloklarin birlesmesini engelledi. Fark 0.
 *    Not: 4 ile 5 birlikte de eslesiyor, yani `goto` gereksiz; yapisal
 *    `if/else` tercih edildi.
 *
 * ESLESME: 164/164 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_a7.c
 */

#include "gba_types.h"

#define SENTINEL_ID 0x7FEF

/* Dugumun isaret ettigi yuva; burada yalniz +0x0C bayrak kelimesi kullaniliyor. */
typedef struct Slot {
    u8  pad00[0x0C];
    u32 flags;                  /* +0x0C */
} Slot;

typedef struct ListNode {
    struct ListNode *next;      /* +0x00 */
    u8    pad04[4];
    u16   id;                   /* +0x08 */
    u8    pad0A[0x0E];
    u32   flags;                /* +0x18 */
    u8    pad1C[0x0C];
    Slot *slot;                 /* +0x28 */
} ListNode;

/* 0x02026F34 / 0x020272C8 icindeki isaretcinin gosterdigi kayit. */
typedef struct Ctx {
    u8        pad00[0x28];
    Slot     *slot;             /* +0x28 */
    u8        pad2C[4];
    ListNode *sel;              /* +0x30 */
} Ctx;

extern ListNode *gList02035A80;     /* 0x02035A80 */

/* GEREKEN SEMBOL BILDIRIMI (yazilmadi, rapor ediliyor): bu ikisi ram_map'te
 * `u32` olarak duruyor ama burada bir Ctx isaretcisi tutuyorlar. Turu
 * degistirmek src/world/node_search.c ile celiskili extern yaratacagi icin
 * mevcut tur korunup adres alindi. */
extern u32 gRam02026F34;            /* 0x02026F34 */
extern u32 gRam020272C8;            /* 0x020272C8 */

extern void FUN_08041ee4(Slot *slot);

/* 0x08053DF8 */
u32 FUN_08053df8(s32 unused, Slot *target, s32 kind)
{
    ListNode *node;
    Ctx *volatile *base;
    Ctx *volatile *primary;
    Ctx *volatile *secondary;
    Slot *slot;

    for (node = gList02035A80; node != 0; node = node->next) {
        if (node->id == SENTINEL_ID)
            break;
        if (node->slot == target)
            break;
    }

    if (node->id == SENTINEL_ID)
        return 0;

    if (node->flags & 0x400) {
        FUN_08041ee4(node->slot);
        node->slot->flags |= 0x10;
    }

    /* Kural 45: dal basina ayri yerel; ortak tek yerel kullanilirsa agbcc
       iki yuklemeyi birlestirip aradaki `b`yi (ve havuz dokumunu) siliyor. */
    if (kind == 2) {
        primary = (Ctx *volatile *)&gRam02026F34;
        base = primary;
    } else {
        secondary = (Ctx *volatile *)&gRam020272C8;
        base = secondary;
    }

    slot = (*base)->slot;
    if (slot->flags & 0x80)
        slot->flags &= ~0x80;
    if (slot->flags & 1)
        slot->flags = (slot->flags & ~0x100) | 0x40;
    (*base)->sel = node;
    return 1;
}
