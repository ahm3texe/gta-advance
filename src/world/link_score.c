/* Baglanti raporundan yuzde skoru — 0x08067014-0x08067183
 *
 * Rapor kaydindaki alanlar uzerinde bir dizi esik kontrolu yapip kac
 * tanesinin tuttugunu sayiyor, sonra `(100 * (bas + sayi)) / (basAlt + 23)`
 * yuzdesini dondururuyor. Sonuc 99'da kirpiliyor; esik zaten asilmissa
 * dogrudan 100 doner.
 *
 * Istatistik blogu 0x08066AA8'de kayit tamponunun +0x64'unden kopyalanan
 * 36 bayt. Icindeki 5 bitlik sayaclar BumpRankCounter (0x08066C94) ile
 * ayni yerlesim: rapor +0x2E, gSaveBuffer +0x7E'nin ta kendisi.
 *
 * Bolme dogrudan __divsi3 cagrisi olarak yazildi; `/` operatoru
 * baska bir yardimci uretiyor (src/text/text_f5.c'deki olcum).
 *
 * DURUM: PARK — 332/368 bayt, 85/183 komut ayni. Yapisi ve esik zinciri
 * dogru; kalan fark yazmac dagitimi.
 *
 * SON TURDA KAPATILAN IKI SINIF:
 *   - `rankA/B/C > 19` ISARETSIZ olmali (`> (u32)19`). Alan `u32 : 5`
 *     olmasina ragmen `int`'e yukseldigi icin duz `> 19` ISARETLI `ble`
 *     uretiyordu; ROM'da `bls` var (kural 56'nin doyum satiri). lapA/B/C
 *     ayni yazimla zaten `bls` uretiyor, cast gerekmiyor.
 *   - Kuyruk blogunun SIRASI: `if (total >= limit) return 100;` sonra duz
 *     hesap. Onceki `if (total < limit) { hesap } return 100;` yazimi
 *     hesabi one aliyordu; ROM once 100 donusunu yerlestiriyor
 *     (`blt <hesap> / movs r0,#100 / b <son>`). Kuyruk artik tam ayni.
 *
 * KALAN TEK SINIF (36 bayt = ~18 komut): ROM `report` isaretcisini
 * `ip`'de (r12) tutup her erisimden once dusuk bir yazmaca kopyaliyor
 * (`mov r0, ip`); biz onu r3'te tutuyoruz ve o 15 kopya komutu hic
 * uretmiyoruz. Sebep dagitim tablosunda gorunuyor (dump_alloc):
 *   - lap ucllusunde biz de ROM gibi `<<` ARA sonucunu canli tutuyoruz
 *     (109/114/121 -> r6/r5/r4) ve toplamlarda `lsrs`i yeniden uretiyoruz;
 *     bu blok ROM ile bire bir ayni.
 *   - rank ucllusunde ise CSE `(x<<k)>>27` ifadesinin TAMAMINI birlestirip
 *     CIKARILMIS degeri canli tutuyor (95/98/103 -> r12/r8/r1) ve iki
 *     toplam testini de tek hesaba indiriyor; ROM her ikisini yeniden
 *     hesapliyor. Fark kabin genisliginden geliyor: lap alanlari `u16`
 *     kapta (HImode ara donusumleri CSE'yi kiriyor), rank alanlari `u32`
 *     kapta. rankB 13-17. bitleri kapsadigi icin kap `u32` OLMAK ZORUNDA
 *     (kural 61; ROM `ldr r0,[r1,#32]` yapiyor), yani bu kolu kaynak
 *     tarafindan cevirmek mumkun degil. Dusuk yazmaclar bosaldigi icin
 *     `report` r3'te kaliyor ve ROM'un `ip` bicimi cikmiyor.
 *
 * BULUNAN KOL (uygulandi): bir `u8` alani birden fazla ifadede
 * kullaniyorsan ONCE YERELE al. Dogrudan uye erisimi agbcc'ye gereksiz
 * `lsls #24 / lsrs #24` sifir-genisletme cifti urettiriyor; yerel bunu
 * kaldiriyor. queryA ikilisinde olculdu.
 *
 * ELENEN YAZIMLAR (bu turda olculdu):
 *   - Ayni kolu queryB dortlusune uygulamak (curB/altB yerelleri):
 *     85 -> 59 komut. ROM +0x06 ve +0x07'yi YENIDEN OKUDUGU icin orada
 *     dogrudan uye erisimi sart — kural 55'in ters yonu dogrulandi.
 *   - Toplam testlerinden `(s32)` cast'ini kaldirmak: degisiklik yok (85).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/link_score.c
 */

#include "gba_types.h"

#define SCORE_FULL   100
#define SCORE_CAP    99
#define LIMIT_BONUS  23

typedef struct ScoreStats {
    u8  pad00[0x0C];
    u32 lowByte : 8;            /* +0x0C bit 0-7 */
    u32 rankA   : 5;            /* bit 8-12 */
    u32 rankB   : 5;            /* bit 13-17 */
    u32 rankC   : 5;            /* bit 18-22 */
    u32 highBits: 9;            /* bit 23-31 */
    u8  pad10[0x18 - 0x10];
    u16 lowNib  : 4;            /* +0x18 bit 0-3 */
    u16 distance: 7;            /* bit 4-10 */
    u16 highPad : 5;            /* bit 11-15 */
    u16 spare   : 1;            /* +0x1A bit 0 */
    u16 lapA    : 5;            /* bit 1-5 */
    u16 lapB    : 5;            /* bit 6-10 */
    u16 lapC    : 5;            /* bit 11-15 */
    u8  pad1C[0x24 - 0x1C];
} ScoreStats;

typedef struct ScoreReport {
    u16        head;            /* +0x00 */
    u16        headAlt;         /* +0x02 */
    u8         queryA;          /* +0x04 */
    u8         queryAAlt;       /* +0x05 */
    u8         queryB;          /* +0x06 */
    u8         queryBAlt;       /* +0x07 */
    u8         g5v1;            /* +0x08 */
    u8         g5v1Alt;         /* +0x09 */
    u8         g5v2;            /* +0x0A */
    u8         g5v2Alt;         /* +0x0B */
    u8         g5v3;            /* +0x0C */
    u8         g5v3Alt;         /* +0x0D */
    u8         summary;         /* +0x0E */
    u8         g7v2;            /* +0x0F */
    u8         g7v2Alt;         /* +0x10 */
    u8         pad11[3];
    ScoreStats stats;           /* +0x14 */
} ScoreReport;

typedef struct SaveNibble {
    u8 pad00[0x7C];
    u8 nibble : 4;              /* +0x7C bit 0-3 */
    u8 rest   : 4;
} SaveNibble;

extern SaveNibble gSaveBuffer;
extern s32 __divsi3(s32 dividend, s32 divisor);

/* 0x08067014 */
s32 FUN_08067014(ScoreReport *report)
{
    s32 count;
    s32 limit;
    s32 total;
    s32 result;
    u32 curA;
    u32 altA;

    count = 0;
    if (report->stats.distance > 49)
        count = 1;
    if (report->stats.distance > 99)
        count++;

    altA = report->queryAAlt;
    curA = report->queryA;
    if (curA >= altA >> 1)
        count++;
    if (curA >= altA)
        count++;

    if (report->queryB >= report->queryBAlt)
        count++;
    if (report->queryB >= report->queryBAlt >> 2)
        count++;
    if (report->queryB >= report->queryBAlt >> 1)
        count++;
    if (report->queryB >= (s32)(report->queryBAlt * 3) >> 2)
        count++;

    if (report->g5v1 >= report->g5v1Alt)
        count++;
    if (report->g5v2 >= report->g5v2Alt)
        count++;
    if (report->g5v3 >= report->g5v3Alt)
        count++;

    if (report->stats.rankA > (u32)19)
        count++;
    if (report->stats.rankB > (u32)19)
        count++;
    if (report->stats.rankC > (u32)19)
        count++;

    if (report->stats.lapA > 19)
        count++;
    if (report->stats.lapB > 19)
        count++;
    if (report->stats.lapC > 19)
        count++;

    if ((s32)(report->stats.rankA + report->stats.rankB + report->stats.rankC) > 59)
        count++;
    if ((s32)(report->stats.rankA + report->stats.rankB + report->stats.rankC) > 29)
        count++;

    if ((s32)(report->stats.lapA + report->stats.lapB + report->stats.lapC) > 59)
        count++;
    if ((s32)(report->stats.lapA + report->stats.lapB + report->stats.lapC) > 29)
        count++;

    if (gSaveBuffer.nibble > 11)
        count++;

    if (report->g7v2 != 0)
        count++;

    limit = report->headAlt + LIMIT_BONUS;
    total = report->head + count;

    if (total >= limit)
        return SCORE_FULL;

    result = __divsi3(SCORE_FULL * total, limit);
    if (result > SCORE_CAP)
        result = SCORE_CAP;
    return result;
}
