/* Odak blogunu yeni hedefle kurma -- 0x0800A8A8-0x0800A92F  (136 bayt, ESLESTI)
 *
 * gSlotSelector 1 ise 1 ve 2 numarali yuvalar TAKAS ediliyor; sonra hedef
 * ya "tutulan" yuvaya (slot == 2, onceki tutulani FUN_0803220c'ye devrederek)
 * ya da CoordBlock +0x04'e yaziliyor. slot <= 1 ise blok ilklendiriliyor:
 * +0x00 = 1, +0x3C/+0x40/+0x44 = 0, +0x0C = kod << 16, +0x71 = ucuncu
 * parametre.
 *
 * Kod, hedefin +0x08 bayrak baytina gore iki kaynaktan geliyor:
 *   0x30 biti kuruluysa  -> (*(hedef+0x20))[+0x1F] alt IKI biti, << 8
 *   degilse              -> (*(hedef+0x18))[+0x0E] halfword & 0x3FF
 *
 * Kural 35: sondaki `pop {r0}; bx r0` donus tipinin void oldugunu soyluyor.
 * Kural 31: `cmp r5,#1 / bhi` ISARETSIZ dal, yani slot parametresi u32.
 *
 * ================= FARKI KAPATAN DORT OLCUM =================
 *
 * (1) UCUNCU PARAMETRE `u32` OLMALI, `u8` DEGIL (kural 15).  144 -> 136 bayt.
 *     `u8 value` girise `lsls r2,#24 / lsrs r7,r2,#24` normalizasyonu
 *     sokuyordu; ROM'da yalnizca `adds r7,r2,#0` var.  Alan (+0x71) yine
 *     u8 oldugu icin store hala `strb`.
 *
 * (2) ALT IKI BIT BITFIELD OLARAK YAZILMALI (kural 24).  140 -> 136 bayt,
 *     fark 54 -> 19.
 *     ROM: `ldrb r0,[r0,#31] / lsls r0,#30 / lsrs r2,r0,#22` -- yani
 *     (alan & 3) << 8 TEK bir kaydirma ciftine kaynasmis.  Acik maskeyle
 *     (`(x & 3) << 8`) agbcc dort komut uretiyor: `movs r0,#3 / ldrb r1 /
 *     ands r0,r1 / lsls r1,r0,#8`.  `u8 unk1F : 2;` bildirimi zero_extract
 *     uretip kaynasmayi sagliyor.
 *
 * (3) BLOK TABANI YEREL ISARETCIYE ALINMAMALI (kural 50 uygulamasi).
 *     fark 19 -> 7.
 *     `CoordBlock *block = &gRam02011030;` yazimi tabani GLOBAL dagiticiya
 *     dusuruyordu; dump_alloc: refs 7 / omur 54 -> oncelik 0.259, sirada
 *     4. siraya kaliyor ve r2'yi aliyordu.  `code` (refs 5 / omur 8 ->
 *     1.250) once dagitilip r1'i kapiyordu -- ROM'un TERSI.
 *     Uyeye DOGRUDAN `gRam02011030.alan` diye erisince taban adresi tek
 *     temel bloga hapsoluyor, YEREL dagiticiya (L11) dusuyor ve r1'i
 *     dagitim yarisindan ONCE aliyor; `code` geriye kalan r2'yi aliyor.
 *     Ikisi de kendiliginden ROM'a oturuyor.
 *
 * (4) `unk18` OKUMASI AYRI DEYIME ALINMALI (kural 18/25).  fark 4 -> 0.
 *     ROM sirasi: `ldr r0,[r6,#24] / ldr r2,=0x3ff / ldrh r0,[r0,#14] /
 *     ands r2,r0` -- isaretci ONCE, sabit SONRA.  `code = 0x3FF;
 *     code &= actor->unk18->unk0E;` sabiti one aliyordu.  `info =
 *     actor->unk18;` ara deyimi ROM'un sirasini veriyor.
 *
 * ============== DENENIP ELENEN YAZIMLAR (tekrar etmeyin) ==============
 *
 * - TEK ORTAK `block` YERELI (blok 2 ve blok 3 icin ayni degisken, iki
 *   ayri atama).  ROM iki ayri `ldr r1,=0x02011030` uretiyor diye umut
 *   vericiydi; olcum tersini soyledi: tek pseudo refs 10 / omur 128 ->
 *   oncelik 0.234, daha da geriye dusuyor.  fark 19, degismedi.
 * - `mask = 0x30; mask &= actor->kind; if (mask != 0)` (kural 33 bicimi):
 *   dogru ama GEREKSIZ.  `if ((actor->kind & 0x30) != 0)` da AYNI
 *   `movs r0,#48 / ldrb r2 / ands r0,r2 / cmp r0,#0` dizisini uretiyor;
 *   kisa olani secildi.
 * - Blok 2 ve blok 6 icin yerel taban isaretcisi: blok 6'da (cagri dali)
 *   yerel de dogrudan erisim de ESLESIYOR -- oradaki taban zaten cagriyi
 *   astigi icin callee-saved r4'e gidiyor, yazim bicimi fark etmiyor.
 *   Blok 2'de ise yerel isaretci r0 veriyordu, ROM r1 istiyor: dogrudan
 *   erisim sart.
 *
 * NOT -- `if (slot == 0) gRam02011030.unk08 = 0;` satiri ROM'da
 * `str r5,[r1,#8]` olarak cikiyor, yani sabit 0 yerine slot yazmaci
 * kullaniliyor.  Bunu biz yazmadik: CSE dal kosulundan slot == 0
 * esitligini kaydedip sifiri o yazmacla degistiriyor.
 *
 * CoordBlock tanimi src/core/read_triple.c, src/core/update_focus.c,
 * src/core/clear_coord_byte71.c, src/misc/coord_accessors.c ve
 * src/misc/coord_more.c ile BIREBIR AYNI olmali (TYPES-001).  Bu yuzden
 * +0x3C/+0x40/+0x44 kelimeleri govdeden oyulmadi, pad3C icinden cast ile
 * yaziliyor; govdeyi degistirmek o bes dosyayi kirardi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/clip_bounds_b1.c
 */

#include "gba_types.h"

typedef struct CoordBlock {
    u32 unk00;                  /* +0  */
    u32 unk04;                  /* +4  */
    u32 unk08;                  /* +8  */
    s32 second;                 /* +12 */
    s32 first;                  /* +16 */
    u8  pad14[28];
    u32 a;                      /* +0x30 */
    u32 b;                      /* +0x34 */
    u32 c;                      /* +0x38 */
    u8  pad3C[12];
    u32 unk48;                  /* +72 */
    u8  pad4C[37];
    u8  byte71;                 /* +0x71 */
} CoordBlock;

/* Hedefin +0x18 ve +0x20 isaretcilerinin gosterdigi bloklarin YALNIZCA
   burada okunan alanlari acildi; gerisi dolgu, anlami bilinmiyor. */
typedef struct InfoA {
    u8  pad00[14];
    u16 unk0E;                  /* +0x0E */
} InfoA;

typedef struct InfoB {
    u8  pad00[31];
    u8  unk1F : 2;              /* +0x1F, alt iki bit (olcum 2) */
} InfoB;

typedef struct Actor {
    u8     pad00[8];
    u8     kind;                /* +0x08 */
    u8     pad09[15];
    InfoA *unk18;               /* +0x18 */
    u8     pad1C[4];
    InfoB *unk20;               /* +0x20 */
} Actor;

extern CoordBlock gRam02011030;
extern u16        gSlotSelector;

extern void FUN_0803220c(Actor *actor, Actor *previous);

/* 0x0800A8A8 */
void FUN_0800a8a8(Actor *actor, u32 slot, u32 value)
{
    u32 sel;

    sel = gSlotSelector;
    if (sel == 1) {
        if (slot == 2)
            slot = 1;
        else if (slot == 1)
            slot = 2;
    }

    if (slot == 2) {
        FUN_0803220c(actor, (Actor *)gRam02011030.unk08);
        gRam02011030.unk08 = (u32)actor;
    } else {
        if (slot == 0)
            gRam02011030.unk08 = 0;
        gRam02011030.unk04 = (u32)actor;
    }

    if (slot <= 1) {
        InfoA *info;
        u32 code;

        gRam02011030.unk00 = 1;
        *(u32 *)&gRam02011030.pad3C[0] = 0;
        *(u32 *)&gRam02011030.pad3C[4] = 0;
        *(u32 *)&gRam02011030.pad3C[8] = 0;

        if ((actor->kind & 0x30) != 0) {
            code = actor->unk20->unk1F << 8;
        } else {
            info = actor->unk18;
            code = 0x3FF;
            code &= info->unk0E;
        }

        gRam02011030.second = code << 16;
        gRam02011030.byte71 = value;
    }
}
