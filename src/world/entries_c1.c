/* Kayittaki iki eksenin orta noktasini aktorun acisiyla dondurup yonelim
 * baytlarina yazar -- 0x08029390-0x080293F7, 104 bayt, Thumb.  ESLESIYOR.
 *
 * Islev, src/world/state_offset.c (0x080260A8) icindeki her dalin ilk
 * yarisiyla ayni iskelet:
 *
 *   1. Kayittan (Record) iki eksen icin orta nokta cikarilir:
 *      (x1 + x0) - (ox - 2), sonra `<< 23 >> 24` ile 9 bitten isaret
 *      genisletilip ikiye bolunur.  Ayni sey y ekseni icin +0x07/+0x05
 *      ve +0x19 ile.
 *   2. Aktorun +0x68'deki 16.16 acisi >> 16 ile tablo indeksine cevrilip
 *      FUN_08029088'e verilir; yardimci iki isaretli bayti yigina yazar.
 *   3. Donen iki bayt 41/32 ile olceklenip aktorun +0x26 / +0x27
 *      yonelim baytlarina yazilir.
 *
 * state_offset.c'nin dallarindan farki: aci MASKELENMIYOR ve aktore geri
 * YAZILMIYOR, ikinci FUN_08029088 cagrisi ve +0x2A kip bayti yok.  Yani bu,
 * o ailenin cikarilmis ortak cekirdegi gibi duruyor.
 *
 * ROM'DAN OLCULEN AYRINTILAR
 *
 *  - Donus tipi void (kural 35): epilog `pop {r4,r5}; pop {r0}; bx r0`,
 *    r0 cagri sonrasi olu.
 *  - Iki parametre: r0 aktor (+0x26/+0x27/+0x68), r1 kayit (+4..+7, +0x18,
 *    +0x19).  Ikisi de ham kelime olarak geliyor, giris daraltmasi yok.
 *  - Sinir alanlari `ldrb` ile okunuyor, isaret genisletme yok -> u8.
 *    Aci `ldr` + `asrs #16` -> isaretli 16.16 kelime.
 *  - Cikti yuvalari sp+4 ve sp+5, yani BITISIK iki bayt -- entries_b4.c ve
 *    state_offset.c'deki ayni cagri kalibi.  `movs r1,#0; ldrsb r1,[r0,r1]`
 *    Thumb'da s8 okumanin tek bicimi (LDRSB'nin immediate ofseti yok).
 *  - 41/32 olcegi shift zincirinden okundu: lsls#2 / adds / lsls#3 / adds
 *    = x*41, sonra asrs#5.  state_offset.c ve entries_b4.c ile ayni sabit.
 *  - `adds r1,r4,#38; strb r0,[r1,#0]`: STRB'nin 5 bitlik immediate ofseti
 *    31'de bittigi icin 38/39 adresleri ayrica kuruluyor.  Kaynakta bir
 *    kaldirac degil, sonuc.
 *
 * ESLESMEYI SAGLAYAN UC OLCUM (41 -> 17 -> 15 -> 0 bayt fark)
 *
 *  1. KAYDIRMALAR CAGRI ARGUMANINDA, YERELDE DEGIL.  ROM once iki eksenin
 *     HAM toplamini cikariyor, aciyi yukluyor, `<<23 >>24` ciftlerini
 *     ANCAK ONDAN SONRA pes pese yapiyor.  Toplami ve kaydirmayi tek yerel
 *     atamasinda birlestirmek (`dx = (s32)((...) << 23) >> 24;`)
 *     kaydirmalari toplamlarin arasina sokuyor: 41 bayt fark.  Kaydirmayi
 *     argumana tasiyinca 17'ye iniyor.
 *     NOT: state_offset.c'nin basligi ayni degisikligin ORADA kotulestirdigini
 *     yaziyor (1430 -> 1178).  Celiski degil: orada aci ayrica maskelenip
 *     aktore geri yaziliyor, yani kaydirmalarin arasina baska is giriyor.
 *     Kardesin bicimini kopyalama, ROM'dan oku.
 *
 *  2. TOPLAMA OPERANDLARININ SIRASI TERS YAZILIR.  agbcc toplamanin IKINCI
 *     operandini ONCE yukluyor.  ROM `ldrb [r1,#6]` (x1) ile basliyor, yani
 *     kaynakta `rec->x0 + rec->x1` yazili.  Duz sira 2 bayt daha birakiyor.
 *
 *  3. `-2` SABITI YERELE ALINIR (kural 44).  Bu belirleyiciydi.
 *     `(x0 + x1) - (rec->ox - 2)` yazildiginda agbcc fold asamasinda
 *     ifadeyi yeniden birlestiriyor: `adds r3,#2`, sonra `ldrb`, sonra
 *     `subs`.  ROM'un sirasi tersi -- `ldrb r0,[r1,#24]; subs r0,#2;
 *     subs r3,r3,r0` -- yani cikarma YUKLENEN degere uygulaniyor.  Sabiti
 *     `two` yereline alinca fold atlaniyor, sabit yine immediate olarak
 *     yayiliyor ve ROM'un dizilimi birebir cikiyor: 15 -> 0.
 *     Bu, state_offset.c'nin basliginda "tek basina etkisi ayrica
 *     olculmeli" diye birakilan 2 numarali fikrin cevabidir: ara YEREL
 *     ise yaramiyor, ara SABIT yariyor.
 *
 * DENENIP ELENEN YAZIMLAR (silme, ekle)
 *
 *  - `t = rec->ox - 2; dx = toplam - t;` ara yereli: 22 bayt.  Ayri deyim
 *    fold'u ENGELLEMIYOR, ustelik t'nin omru dagitimi da kaydiriyor.
 *    `two` ile birlikte kullanmak da 22'de kaliyor -- `t` yereli zararli.
 *  - `dx = toplam + (2 - rec->ox);` : 15, yani duz cikarma ile ayni.
 *    Fold her iki yazimi da ayni agaca indiriyor.
 *  - Kaydirmalari ayri deyimde yapmak (`dx = (dx << 23) >> 24;`): 28 bayt.
 *  - Aciyi once `ang` yereline almak: 17'de degisiklik yok (fark 2 ve 3
 *    hala acikken olculdu); esleseme ulasildigi icin tekrar denenmedi.
 *  - `two` yerine iki ayri yerel (`twox`/`twoy`) da 0 veriyor; tek yerel
 *    daha sade oldugu icin o tutuldu.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_c1.c   -> 104/104
 */

#include "gba_types.h"

#define SCALE_NUM  41           /* 41/32 ~ 1.28, state_offset.c ile ayni */
#define SCALE_SH   5

/* state_offset.c'deki Record gorunumunun aynisi: iki eksen icin sinir
 * cifti (+0x04..+0x07) ve iki origin bayti (+0x18/+0x19).  Aradaki 16
 * bayt bu ceviri biriminde okunmuyor, pad birakildi. */
typedef struct Record {
    u8 pad0[4];
    u8 x0;                /* +0x04 */
    u8 y0;                /* +0x05 */
    u8 x1;                /* +0x06 */
    u8 y1;                /* +0x07 */
    u8 pad8[0x10];
    u8 ox;                /* +0x18 */
    u8 oy;                /* +0x19 */
} Record;

/* state_offset.c ve entries_b4.c ile ayni ofsetler; bu ceviri biriminde
 * yalnizca yonelim baytlari ve aci okunuyor. */
typedef struct Actor {
    u8  pad0[0x26];
    s8  fx;               /* +0x26 */
    s8  fy;               /* +0x27 */
    u8  pad28[0x40];
    s32 angle;            /* +0x68, 16.16 */
} Actor;

/* Imza state_offset.c'de ROM'dan dogrulandi. */
extern void FUN_08029088(s32 angle, s32 dx, s32 dy, s8 *outX, s8 *outY);

/* 0x08029390 */
void SetActorOffsetFromRecord(Actor *actor, Record *rec)
{
    s32 dx;
    s32 dy;
    s32 two;
    s8  ox;
    s8  oy;

    /* Kural 44: sabit yerele alinmazsa agbcc `- (ox - 2)` ifadesini
     * `+ 2 - ox` diye yeniden birlestiriyor ve komut sirasi kayiyor. */
    two = 2;
    dx = (rec->x0 + rec->x1) - (rec->ox - two);
    dy = (rec->y0 + rec->y1) - (rec->oy - two);

    /* Kaydirmalar bilerek argumanda: ROM ham toplamlari once cikariyor. */
    FUN_08029088(actor->angle >> 16, (dx << 23) >> 24, (dy << 23) >> 24,
                 &ox, &oy);

    actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
    actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
}
