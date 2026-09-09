/* Odak noktasi guncelleme - 0x0800A9E4-0x0800AA3B  (88 bayt, ESLESTI)
 *
 * gGameState +0x0C bayragi kuruluysa VE ikinci hedef varsa iki hedefin
 * konumlarinin ORTA NOKTASINI, aksi halde birinci hedefin konumunu
 * gFocusPoint'e yaziyor. Bolme `asrs #1`, yani isaretli.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * CoordBlock tanimi src/misc/coord_accessors.c ile BIREBIR AYNI olmali;
 * +0x04 ve +0x08 hedef isaretcisi olarak cast ediliyor.
 *
 * Izleme logu (docs/GAME_FLOW.md) gFocusPoint'in iki adet 16.16 sabit
 * nokta s32 oldugunu OLCTU (baslangic 3360.000 / 9568.000; bazi farklar
 * tam 65536 ve 262144). Buradaki `Vec2 {s32 x; s32 y;}` tanimini
 * DOGRULADI.
 *
 * ================= FARKI KAPATAN IKI OLCUM =================
 *
 * (1) KURAL 45 -- DAL BASINA AYRI YERELLER.  77 -> 26 bayt fark.
 *     Onceki yazim iki dalda da ayni `out` ve `first` yerellerini
 *     kullaniyordu.  Iki dalin son komutu (`str r0,[out+4]`) o yuzden
 *     RTL'de AYNI insn oluyordu ve `jump` gecisi CROSS-JUMPING ile
 *     ikisini birlestiriyordu: cikti 84 bayt kaliyordu (ROM 88), dusme
 *     dali sondaki store'a `b` ile atliyordu.  Dallara ayri yereller
 *     (`out`/`mout`, `first`/`mfirst`) verilince birlesme imkansiz
 *     oldu; boyut 88'e ciktI ve YAZMAC DAGITIMI DA kendiliginden ROM'a
 *     oturdu (block r2->r3, out r4->r2, konum isaretcisi r0->r1).
 *     Yani "kalan fark yazmac dagitimi" teshisi YANLISTI; tek sebep
 *     ortak yereldi.
 *
 * (2) TABAN KOPYASI `adds r0,r3,#0`.  26 -> 0 bayt fark.
 *     ROM tabani AYRI bir yazmaca kopyalayip iki yerde o kopyayi
 *     kullaniyor:
 *         800a9f0  adds r0,r3,#0      <- kopya
 *         800a9f2  ldr  r1,[r0,#8]    <- kopyadan (bayrak dali)
 *         800aa16  ldr  r0,[r0,#4]    <- kopyadan (orta nokta dali)
 *         800a9fa  ldr  r0,[r3,#4]    <- OZGUN tabandan (dusme dali)
 *     Kopyayi ureten sey `probe = block;` DEGIL (asagiya bak), SEMBOLU
 *     IKI KEZ YAZMAK:  bayrak dalinda `probe = &gRam02011030;`, dusme
 *     dalinda `block = &gRam02011030;`.  Iki referans FARKLI CSE
 *     bloklarinda oldugu icin CSE ikisini tek yazmaca indirgemiyor;
 *     ortak alt ifade sonradan havuz yuklemesini dal oncesine tasiyor
 *     ve bayrak dalina reg-reg kopyasini birakiyor.  Sonuc tam olarak
 *     ROM'un sekli: havuz yuklemesi 0x800a9ea'da (dal oncesi, dusme
 *     dalina hizmet ediyor), kopya 0x800a9f0'da (bayrak + orta nokta
 *     dallarina hizmet ediyor).
 *
 * ============== DENENIP ELENEN YAZIMLAR (tekrar etmeyin) ==============
 *
 * KOPYA SINIFI -- hepsi kopyayi YOK ETTI, cikti degismedi (fark 26):
 *   - `probe = block;`                       (uc ayri oturumda denendi)
 *   - `probe = block + 0;`
 *   - `probe = (CoordBlock *)((char *)block + 0);`
 *   - `probe = &gRam02011030;` AMA `block` da ayni EBB'de kaliyorken
 *     (H_twoRefs_order: `block = &gRam02011030;` dal ONCESINE alinmis).
 *     KRITIK: iki referansin ayri CSE bloklarinda olmasi SART; ikisi de
 *     giris blogunda olursa CSE tek yazmaca indirir.
 *   - `probe = 0;` ile on-atama (olu kod, DCE siliyor).
 *   - Bildirim SIRASI permutasyonlari (block/probe/flag once, probe en
 *     sonda, block en sonda -> BESI DE fark 26).  Pseudo numaralari
 *     CSE'nin kanoniklestirme yonunu DEGISTIRMIYOR.
 *
 * CFG SINIFI -- hepsi fark 26 (yani CFG'yi degistirmek tek basina
 * yetmiyor; base26 ile ayni RTL cikiyor):
 *   - `if (flag == 0 || (second = probe->unk08) == 0) { dusme; return; }`
 *   - Ayni sey virgul operatoruyle: `(probe = block, second = ...)`
 *   - `if (... && ...) goto midpoint;`
 *   - `if (second == 0) goto simple; goto midpoint;` ikili goto
 *
 * MERGE SINIFI -- kopyayi YASATTI ama 2 bayt PAHALI (fark 81, 92 bayt):
 *   - `second = 0;` ile ikinci testi if govdesinden CIKARMAK.  Araya
 *     cok referansli bir code_label giriyor, CSE blogu orada bitiyor,
 *     kopya yasiyor.  Ama `movs r1,#0` fazladan 2 bayt + havuz hizasi
 *     icin 2 bayt dolgu getiriyor.  Dogru mekanizma, yanlis bedel.
 *   - `second`i `&gRam02011030` gibi bir nobet degeriyle merge etmek:
 *     96 bayt, fark 86.  Cok daha kotu.
 *
 * TESHIS ARACI (baskasi ayni duvara toslarsa): agbcc `-da` bayragini
 * KABUL EDIYOR.  `old_agbcc -mthumb-interwork -O2 -fhex-asm -da -o x.s
 * x.i` her gecis icin bir RTL dokumu birakiyor (x.i.rtl, x.i.jump,
 * x.i.cse, x.i.gcse, x.i.loop, x.i.cse2, x.i.flow, x.i.combine,
 * x.i.regmove, x.i.lreg, x.i.greg).  Kopyanin nerede oldugunu bununla
 * OLCTUK: `probe = block` insn 9 olarak rtl'de duruyor, CSE her iki
 * kullanimini da `block`a ceviriyor (insn 24 ve insn 61), `flow`
 * gecisi olu kalan kopyayi siliyor.  `-fno-cse-follow-jumps` ile de
 * silinmesi, sebebin dal takibi DEGIL ayni blok icindeki
 * kanoniklestirme oldugunu gosterdi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/update_focus.c
 */

#include "gba_types.h"
#include "game_state.h"

typedef struct Vec2 {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
} Vec2;

typedef struct Target {
    u8    pad00[24];
    Vec2 *pos;                  /* +0x18 */
} Target;

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

extern CoordBlock gRam02011030;

extern Vec2       gFocusPoint;

/* 0x0800A9E4 */
void UpdateFocusPoint(void)
{
    CoordBlock *block;
    CoordBlock *probe;
    Target *first;
    Target *second;
    Target *mfirst;
    Vec2 *out;
    Vec2 *mout;
    u32 flag;

    /* SIRA: ROM once gGameState bayragini OKUYOR.  Blok tabani, bayrak
       dalinda ve dusme dalinda AYRI AYRI yaziliyor (yukaridaki olcum 2):
       iki referans ayri CSE bloklarinda oldugu icin ROM'un
       `ldr r3,=blok` + `adds r0,r3,#0` ikilisi cikiyor. */
    flag = gGameState.flag;
    if (flag != 0) {
        probe = &gRam02011030;
        second = (Target *)probe->unk08;
        if (second != 0)
            goto midpoint;
    }

    /* Dusme dali: ozgun tabandan okuyor (ROM: ldr r0,[r3,#4]).
       Kural 45 -- bu dalin yerelleri orta nokta dalindan AYRI olmali,
       yoksa sondaki store cross-jumping ile birlesiyor. */
    block = &gRam02011030;
    out = &gFocusPoint;
    first = (Target *)block->unk04;
    out->x = first->pos->x;
    out->y = first->pos->y;
    return;

midpoint:
    /* Orta nokta dali: +0x04 KOPYADAN okunuyor (ROM: ldr r0,[r0,#4]).
       Kural 49 -- ROM bu seyrek govdeyi fonksiyonun SONUNDA tutuyor. */
    mout = &gFocusPoint;
    mfirst = (Target *)probe->unk04;
    mout->x = (mfirst->pos->x + second->pos->x) >> 1;
    mout->y = (mfirst->pos->y + second->pos->y) >> 1;
}
