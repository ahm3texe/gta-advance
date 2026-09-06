/* Ilerleme kosulu sorgusu — 0x080552D4-0x080554BF (492 bayt)
 *
 * TEK PARAMETRELI PREDIKAT: 0..23 arasindaki bir kosul kimligini alip
 * 0 veya 1 dondurur. ROM'da 24 girisli ATLAMA TABLOSU var
 * (0x080552EC-0x0805534B, 96 bayt), yani kaynak yogun bir `switch`:
 *
 *   push {lr} / cmp r0,#23 / bls .L / b default / lsls r0,#2 /
 *   ldr r1,=0x080552EC / adds r0,r0,r1 / ldr r0,[r0] / mov pc,r0
 *
 * `cmp #23` sonrasi `bls .L + b default` ikilisi elle yazilmis bir sey
 * degil: default blogu (0x080554BA) 482 bayt uzakta ve Thumb kosullu dal
 * +-254 bayta sigmiyor, agbcc kendi trambolinini uretiyor. Ayni sebeple
 * govdelerin cogunda `bls <yakin>` + `b <uzak>` ciftleri var.
 *
 * Tablo girisi 0 -> default (0x080554BA), yani KAYNAKTA `case 0` YOK.
 * Tablo girisi 23 -> ortak `return 0` blogu, yani `case 23: return 0;`
 * capraz atlamayla kuyruga kaynamis.
 *
 * VERI KAYNAGI: gSaveBuffer (0x02000D50, data/ram_map.csv). Uc ayri
 * bitfield kabi okunuyor; genislikler cikarma komutlarindan TERSINE
 * hesaplandi (`(v << (32-bitpos-w)) >> (32-w)` gcc'nin extract_bit_field
 * kalibi):
 *
 *   +0x70  u32 kabi
 *       ldrb [+0x71] / lsls #27 / lsrs #27   -> bit  8, genislik 5
 *       ldr  [+0x70] / lsls #14 / lsrs #27   -> bit 13, genislik 5
 *       ldrb [+0x72] / lsls #25 / lsrs #27   -> bit 18, genislik 5
 *   +0x7C  u16 kabi
 *       ldrb [+0x7C] / lsls #28 / lsrs #28   -> bit  0, genislik 4
 *       ldrh [+0x7C] / lsls #21 / lsrs #25   -> bit  4, genislik 7
 *   +0x7E  u16 kabi
 *       ldrb [+0x7E] / lsls #26 / lsrs #27   -> bit  1, genislik 5
 *       ldrh [+0x7E] / lsls #21 / lsrs #27   -> bit  6, genislik 5
 *       ldrb [+0x7F] / lsrs #3               -> bit 11, genislik 5
 *
 * Son satirdaki tek `lsrs #3`: bit 11..15 baytin ust ucunda bittigi icin
 * gcc maskeyi eleyip yalniz kaydirmayi biraktigi hal. Yani alan 5 bit.
 *
 * KAP GENISLIGI ZORUNLU: +0x7E'deki 6..10 alani BAYT SINIRINI ASIYOR
 * (ldrh ile okunuyor), dolayisiyla bitfield tipi `u8` OLAMAZ; `u16`
 * olmali. Ayni sekilde +0x70'teki 13..17 alani ldr ile okunuyor -> `u32`.
 * +0x7C ile +0x7E AYRI kaplar: bitisik yazilirsa 0x7C kabinda 5 bit bos
 * kalir ve 0x7E'nin ilk alani oraya kayar.
 *
 * OLCUM GUNLUGU -- iki tur, ikisi de tek degiskenli (yeni deneyen BU
 * LISTEYE EKLESIN, mevcut satirlari SILMESIN):
 *
 * 1) `case 0` YOKKEN 488/492 -- DORT BAYT KISA.
 *    Belirti: bizim prologumuz `subs r0,#1 / cmp r0,#22` ve 23 girisli
 *    tablo uretiyordu, ROM'da ise `cmp r0,#23` ve 24 girisli tablo var.
 *    agbcc switch'in en kucuk case'ini cikarir; `subs` YOKSA en kucuk
 *    case SIFIRDIR. Yani kaynakta `case 0:` VAR ve govdesi default ile
 *    ayni (`return 1`) oldugu icin capraz atlamayla kaynamis; tabloda
 *    yalniz fazladan bir kelime birakiyor. `case 0: return 1;` eklemek
 *    488 -> 492 yapti (fark 154).
 *
 * 2) +0x70 KABININ KARSILASTIRMALARI UNSIGNED OLMALI: 154 -> 0.
 *    Olculen agbcc davranisi (probe ile dogrulandi):
 *        u16 alan : 5   ->  `cmp #19 / bhi`   (UNSIGNED)
 *        u32 alan : 5   ->  `cmp #19 / bgt`   (SIGNED)
 *    Yani bitfield'in BILDIRILEN TIPI karsilastirmanin isaretliligini
 *    belirliyor. +0x7C ve +0x7E kaplari `u16` oldugu icin zaten ROM'un
 *    `bhi/bls`sini veriyordu; +0x70 kabi `u32` OLMAK ZORUNDA (bit 13..17
 *    alani halfword sinirini asiyor, `u16` bildirilirse agbcc alani bir
 *    sonraki u16'ya kaydirir ve ofset +0x72'ye kayar), o yuzden isaret
 *    kaynakta duzeltildi: sabitlere `U` soneki.
 *    Elenen esdegerler (ucu de AYNI kodu veriyor, gereksiz):
 *        (u32)gSaveBuffer.score1 > 9      -- acik cast
 *        u32 v = gSaveBuffer.score1; v>9  -- ara yerel
 *        gSaveBuffer.score1 >= 10U        -- >= yazimi
 *
 *    Bu duzeltmenin ZINCIRLEME etkisi vardi, sadece iki komut degil:
 *      - case 3/5/7 ancak isaretlilik tutunca case 1'in `cmp #19` /
 *        case 2'nin `cmp #9` kuyruguna capraz atlayabildi (ROM'da
 *        `b 0x8055372` ve `b 0x8055388` bunlar),
 *      - buna karsilik case 11..20 (FUN_08030390 donusu, SIGNED `bgt`)
 *        yanlislikla o kuyruklara KAYNAMISTI; isaret ayrisinca ROM'daki
 *        gibi kendi `cmp/bgt` ciftlerini geri aldilar.
 *    Tek bir tip kararinin hem birlestirdigi hem ayirdigi bloklar
 *    sayesinde 154 baytin tamami tek hamlede kapandi.
 *
 * ESLESME: 492/492 bayt.
 *
 * DIS SEMBOL: 0x08061EC0 haritada EKSIKTI (onceki kayit 0x08061DF0,
 *   sonraki 0x08061EC4); ROM'da orada 4 baytlik `movs r0,#1 / bx lr`
 *   duruyor -- gercek bir yaprak fonksiyon, iki havuz kelimesinin
 *   arasinda kaldigi icin Ghidra kacirmis. Bildirildi ve
 *   data/functions.csv'ye `0x08061EC0,FUN_08061ec0,4` olarak eklendi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_b8.c
 */

#include "gba_types.h"

/* Kayit tamponunun bu ceviri biriminden gorunen yuzu. Diger gorunumler:
 * src/world/area_flags.c (SaveBuffer), src/world/stat_counters.c
 * (SaveCounters). Her TU kendi yerel gorunumunu bildiriyor. */
typedef struct SaveProgress {
    u8  pad00[0x70];            /* 0x00 */

    u32 unk70_0  : 8;           /* 0x70 bit  0 */
    u32 score1   : 5;           /* 0x70 bit  8 */
    u32 score2   : 5;           /* 0x70 bit 13 */
    u32 score3   : 5;           /* 0x70 bit 18 */
    u32 unk70_23 : 9;           /* 0x70 bit 23 */

    u8  pad74[8];               /* 0x74 */

    u16 tally0   : 4;           /* 0x7C bit  0 */
    u16 tally4   : 7;           /* 0x7C bit  4 */
    u16 unk7C_11 : 5;           /* 0x7C bit 11 */

    u16 unk7E_0  : 1;           /* 0x7E bit  0 */
    u16 lap1     : 5;           /* 0x7E bit  1 */
    u16 lap2     : 5;           /* 0x7E bit  6 */
    u16 lap3     : 5;           /* 0x7E bit 11 */
} SaveProgress;

extern SaveProgress gSaveBuffer;

extern s32 FUN_08030390(s32 a, s32 b);
extern s32 FUN_08061ec0(s32 a);

/* 0x080552D4 */
u32 FUN_080552d4(u32 kind)
{
    switch (kind) {
    case 0:
        return 1;
    case 1:
        if (gSaveBuffer.lap1 > 19 && gSaveBuffer.lap2 > 19
            && gSaveBuffer.lap3 > 19)
            return 1;
        return 0;
    case 2:
        if (gSaveBuffer.score1 > 9U)
            return 1;
        return 0;
    case 3:
        if (gSaveBuffer.score1 > 19U)
            return 1;
        return 0;
    case 4:
        if (gSaveBuffer.score2 > 9U)
            return 1;
        return 0;
    case 5:
        if (gSaveBuffer.score2 > 19U)
            return 1;
        return 0;
    case 6:
        if (gSaveBuffer.score3 > 9U)
            return 1;
        return 0;
    case 7:
        if (gSaveBuffer.score3 > 19U)
            return 1;
        return 0;
    case 8:
        if (gSaveBuffer.tally0 > 7)
            return 1;
        return 0;
    case 9:
        if (gSaveBuffer.tally0 > 9)
            return 1;
        return 0;
    case 10:
        if (gSaveBuffer.tally4 > 49)
            return 1;
        return 0;
    case 11:
        if (FUN_08030390(79, 0) > 9)
            return 1;
        return 0;
    case 12:
        if (FUN_08030390(79, 0) > 19)
            return 1;
        return 0;
    case 13:
        if (FUN_08030390(79, 0) > 29)
            return 1;
        return 0;
    case 14:
        if (FUN_08030390(79, 0) > 39)
            return 1;
        return 0;
    case 15:
        if (FUN_08030390(79, 0) > 49)
            return 1;
        return 0;
    case 16:
        if (FUN_08030390(79, 0) > 59)
            return 1;
        return 0;
    case 17:
        if (FUN_08030390(79, 0) > 69)
            return 1;
        return 0;
    case 18:
        if (FUN_08030390(79, 0) > 79)
            return 1;
        return 0;
    case 19:
        if (FUN_08030390(79, 0) > 89)
            return 1;
        return 0;
    case 20:
        if (FUN_08030390(79, 0) > 99)
            return 1;
        return 0;
    case 21:
        if (FUN_08061ec0(2) != 0)
            return 1;
        return 0;
    case 22:
        if (FUN_08061ec0(3) != 0)
            return 1;
        return 0;
    case 23:
        return 0;
    }

    return 1;
}
