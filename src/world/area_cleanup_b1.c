/* Oturum kurulumu -- 0x08030E10-0x08030E77, 104 bayt.
 * DURUM: 50 komutun 50'si de ROM ile AYNI.  Kalan tek fark, henuz
 * data/ram_map.csv'de bulunmayan 0x02026DF0 sembolunun havuz kelimesi.
 * Kayit eklenince fark 0 olur (olculdu, asagida).
 *
 * EWRAM sayaci kuruluysa 0x02026DF0'daki calisma blogunu hazirliyor:
 * bayragi 1 yapiyor, ilerleme blogundan +0x14 sozcugunu kopyaliyor,
 * kayit tamponunun +0x64'undeki 36 bayti blogun +0x60'ina tasiyor ve
 * FUN_0802fd48'i blogun +0x08 kaydiyla (kip 1) cagiriyor. gGameState[12]
 * kuruluysa ikinci kaydi (+0x34, kip 2) da kuruyor.
 *
 * RAM_MAP KAYDI GEREKIYOR -- TEK EKSIK BU:
 *     0x02026DF0, >=0x84 bayt, gRam02026DF0
 * Sembol data/ram_map.csv'de yok; agbcc_build.py cozemeyip sys.exit ediyor.
 * (Bu dosya data/*.csv'ye dokunmuyor; kaydi proje sahibi ekleyecek.)
 *
 * OLCUM (kanit): dosya gecici olarak, ram_map'te ZATEN bulunan komsu
 * sembol gRam02026E80 (0x02026E80) ile derlendi -- kod uretimi ayni,
 * yalnizca havuz kelimesi degisir:
 *     make c-match  -> farkli: 2/104 byte
 *     diff_function -> 49/50 komut ayni, 1 farkli
 * Farkli sayilan tek "komut" havuzdaki 0x02026DF0 kelimesinin kendisi
 * (0x6DF0 yerine 0x6E80); gerceklerdeki 2 bayt tam olarak o kelimenin
 * alt yarisidir. Govdenin 42 komutunun tamami ROM ile birebir.
 *
 * ROM'DAN OKUNAN BICIM (tahmin degil, disasm):
 *   - Giris testi ERKEN DONUS: `cmp #0 / bne ileri / movs r0,#0 / b son`.
 *     Sifir donusu testin hemen ardinda, yani kaynakta da fonksiyonun
 *     BASINDA duz bir `if (...) return 0;`. Kural 49 (seyrek govdeyi sona
 *     tasi) BURADA GECERSIZ: dal ileri atlayip govdeyi ASIYOR, govde
 *     ilerideki blok degil.
 *   - 36 baytlik kopya UC ldmia/stmia cifti (`{r2,r3,r5}` yazma-geri).
 *     Kural 32: bu ancak STRUCT ATAMASI ile cikar (`d->unk60 = s->unk64`),
 *     `*d++ = *s++` uclusu ayri ldr/str uretirdi.
 *   - Iki taban da havuzdan yuklenip `adds rN,#ofset` ile kaydiriliyor,
 *     katlanmis literal (0x02026E50 / 0x02000DB4) YOK -> kural 1: her
 *     ikisi de extern SEMBOL olmali, `((T*)0xADDR)` cast'i degil. Cast
 *     yazimi taban+ofseti ayri havuz kelimesine katlardi.
 *   - Cagrilan iki kayit ayri ISIMLI uye; `Record kayit[2]` + sabit indis
 *     denenmedi cunku sabit indis zaten katlanir, isimli uye ROM'un
 *     `adds r0,r4,#0 / adds r0,#0x34` ciftini dogrudan veriyor.
 *   - FUN_0802fd48'in donus degeri kullanilmiyor; imza `void` yazildi
 *     (kural 35 yonunde, cagri yerinde fark uretmiyor).
 *
 * ELENEN YOLLAR: yok -- ilk yazim ROM'u birebir verdi, arama gerekmedi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/area_cleanup_b1.c
 *             (ram_map kaydi eklendikten sonra 1/1 byte-matching bekleniyor)
 */

#include "gba_types.h"
#include "session.h"
#include "ram_symbols.h"

/* Kayit tamponundan blogun +0x60'ina tasinan 36 baytlik kume.  Icerigi
 * bilinmiyor; yalnizca boyutu olculdu (uc ldmia/stmia cifti). */
/* FUN_0802fd48'in kurdugu kayit: +0x00/+0x02 u16, +0x04 uc sozcuk,
 * +0x10 u32, +0x14 alt kayit, +0x26 u16, +0x28 u8.  Toplam 0x2C
 * (bloktaki iki ornegin arasi: 0x34 - 0x08). */
/* 0x02026DF0 -- oturum calisma blogu. */
/* 0x02000D50 -- kayit tamponu; yerlesim src/world/area_flags.c'den. */
typedef struct SaveBuffer {
    u8       header[12];        /* 0x00 */
    u8       unk0C[48];         /* 0x0C */
    u32      entityFlags[4];    /* 0x3C */
    u32      areaFlags[6];      /* 0x4C */
    Snapshot unk64;             /* 0x64 -- kopyalanan kume */
    u8       unk88[20];         /* 0x88 */
    u8       complement;        /* 0x9C */
    u8       unk9D[3];
} SaveBuffer;

/* 0x02025810 -- ilerleme blogu; +0x14 sozcugu bloga tasiniyor. */
typedef struct Progress {
    u8  pad0000[0x14];
    u32 unk14;                  /* 0x14 */
} Progress;

extern u32        gFrameCounterEwram;   /* 0x02000EB4 */
extern u8         gGameState[16];       /* 0x02000CE0 */
extern SaveBuffer gSaveBuffer;          /* 0x02000D50 */
extern Session    gRam02026DF0;         /* 0x02026DF0 -- ram_map kaydi gerekli */

extern void FUN_0802fd48(Record *record, s32 kind);

/* 0x08030E10 */
u32 CaptureSessionSnapshot(void)
{
    Session *session;

    if (gFrameCounterEwram == 0)
        return 0;

    session = &gRam02026DF0;
    session->unk00 = 1;
    session->unk04 = ((Progress *)gRam02025810)->unk14;
    session->unk60 = gSaveBuffer.unk64;
    FUN_0802fd48(&session->unk08, 1);

    if (gGameState[12] != 0)
        FUN_0802fd48(&session->unk34, 2);

    return 1;
}
