/* SpawnFollowupEntry — 0x080291B8-0x0802922F (120 bayt)
 *
 * Bir gEntriesA girisini sablon olarak verip ondan tur 34'ten yeni bir
 * giris kuruyor, sonra iki genel bayragi tazeliyor.  Evre secimi girisin
 * turune/evresine bagli:
 *
 *   tur (+0x64) == 76  YA DA  evre (+0x90) == 46
 *       -> CreateEntryFromTemplate(e, 1, 0, 34, 47, e->owner)
 *          FUN_08035058(GetActiveSlotValue(), 461)
 *   degilse
 *       -> CreateEntryFromTemplate(e, 1, 0, 34, 7, e->owner)
 *          FUN_08035058(GetActiveSlotValue(), 258)
 *
 * Her iki durumda da sonunda gRam020245A0 = 1 ve gRam02024344 = 0.
 *
 * KARDES: bu govde, src/world/entries_b5.c (StepEntryPhase, 0x08025518)
 * icindeki ikinci is blogunun BIREBIR aynisi.  Struct yerlesimi, cagri
 * imzalari ve RAM sembolleri oradan alindi; orada zaten byte-matching
 * oldugu icin sabitlerin/argumanlarin dogrulugu ikinci kez kanitli.
 *
 * ROM'DAN OLCULEN AYRINTILAR
 * --------------------------
 *  1. Kural 35 -- DONUS TIPI void.  Epilog `add sp,#8; pop {r0}; bx r0`:
 *     donus adresi r0'a aliniyor, yani r0 canli DEGIL.  Deger dondurse
 *     (kardes StepEntryPhase'de oldugu gibi) `pop {r1}; bx r1` olurdu.
 *
 *  2. PROLOG `push {lr}` -- HIC callee-saved yazmac yok.  `e` yalnizca
 *     `adds r1,r0,#0` ile r1'e aliniyor ve son cagridan ONCE tuketiliyor
 *     (CreateEntryFromTemplate'in argumanlari kurulurken).  Sonraki iki
 *     cagri e'ye dokunmadigi icin agbcc onu caller-saved r1'de tutabiliyor.
 *     Bu, `e`nin kaynakta cagri sinirini asan bir yerele KOPYALANMAMASI
 *     gerektigini soyluyor -- dogrudan parametre kullanildi.
 *     `sub sp,#8` iki yigin argumani (5. ve 6.) icin.
 *
 *  3. ALAN ERISIMLERI `adds rX,#100` / `adds rX,#144` + ofset-0 yukleme
 *     olarak cikiyor; bu Thumb'in zorunlu bicimi (ldrb imm5 <= 31,
 *     ldr imm5*4 <= 124), yani burada kural 2'nin iki yazimi arasinda
 *     secim yok -- duz `e->kind` / `e->phase` dogru.
 *
 *  4. `e->owner` (+0x01) `ldrb` ile okunup `str` ile yigina yaziliyor:
 *     dar alan, genis (u32) parametre yuvasi.  Kardesteki
 *     CreateEntryFromTemplate imzasinin son argumani `u8 owner`; ayni
 *     imza burada da tam bu ldrb/str ciftini uretiyor.
 *
 *  5. IKI BILDIRIM SABITI FARKLI YOLDAN KURULUYOR ve bu KENDILIGINDEN
 *     oluyor, kaynakta bir kaldirac gerekmiyor:
 *       461 = 0x1CD -> imm8<<n olarak kurulamiyor, havuzdan yukleniyor.
 *              Havuz 0x080291F0'ta, yani ilk dalin `b` komutundan HEMEN
 *              SONRA -- agbcc havuzu ilk erisilemez noktaya dokuyor.
 *       258 = 0x81<<1 -> `movs r1,#129; lsls r1,#1`.
 *     Kaynakta ikisi de duz `#define` sabiti; kural 44'e (sabiti yerele
 *     alma) GEREK YOK, cunku burada karsilastirma degil arguman.
 *
 *  6. KOSUL SIRASI kaynak sirasiyla ayni: once `cmp #76` (tur), sonra
 *     `cmp #46` (evre).  `||` zincirinin iki terimi bitisik esitlik
 *     OLMADIGI icin (farkli alanlar) kural 44'un aralik-katlamasi
 *     tetiklenmiyor; kardesteki 46/47 ciftinde gereken `c47` yereli
 *     burada gereksiz.
 *
 *  7. `if/else` KORUNDU, kural 29'a gerek yok: ROM'da iki cagri blogu
 *     ayri ayri duruyor ve ortak kuyruk (iki store + epilog) 0x8029214'te
 *     bulusuyor -- yani agbcc'nin capraz atlamasi burada zaten ROM'un
 *     istedigi seyi yapiyor.  Ilk dal `b 0x8029214` ile kuyruga atliyor,
 *     ikinci dal kuyruga DUSUYOR (kural 49 ile uyumlu yerlesim).
 *
 * DENENIP ELENEN YOLLAR
 * ---------------------
 *  (YOK -- ILK DENEMEDE ESLESTI, tek varyant denenmedi.  Kardes
 *   entries_b5.c'nin ayni bloguna guvenmek yetti: struct yerlesimi,
 *   imzalar, sabitler ve if/else bicimi oradan degistirmeden alindi.
 *   Yeni denemeler BURAYA eklenmeli, mevcut notlar silinmemeli.)
 *
 * ESLESME: 120/120 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_a9.c
 */

#include "gba_types.h"

#define KIND_34    34           /* yeni girisin turu, +0x64 */
#define KIND_76    76           /* sablon girisin turu icin esik */

#define PHASE_7     7           /* CreateEntryFromTemplate'e verilen evre */
#define PHASE_46   46           /* sablon girisin evresi icin esik */
#define PHASE_47   47           /* CreateEntryFromTemplate'e verilen evre */

#define NOTIFY_A  461           /* FUN_08035058 ikinci argumani */
#define NOTIFY_B  258

/* +0x4C'deki 12 baytlik uclu; src/world/entries_b5.c'deki `Triple` ve
 * src/world/submit_pack.c'deki `Pack12` ile ayni nesne.  Bu fonksiyon
 * icerigine dokunmuyor, yalnizca yerlesimi tamamlamak icin duruyor. */
typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

/* Kardes dosyalarla (entries_a5.c, entries_a6.c, entries_b1.c,
 * entries_b5.c) ayni yerlesim; toplam 148 = 0x94. */
typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     owner;               /* +0x01, sablondan yeni girise tasiniyor */
    u8     pad02[2];
    u8     sub[38];             /* +0x04 */
    u8     pad2a[34];
    Triple payload;             /* +0x4C */
    u8     pad58[12];
    u8     kind;                /* +0x64 */
    u8     pad65[31];
    u32    unk84;               /* +0x84 */
    u8     pad88[8];
    u32    phase;               /* +0x90, stride 148 */
} Entry;

/* Adresleri data/ram_map.csv'de kayitli (0x020245A0 ve 0x02024344);
 * derleme katmani `.equ` bildirimlerini kendisi uretiyor. */
extern u8  gRam020245A0;
extern u16 gRam02024344;

extern u32  GetActiveSlotValue(void);
extern void FUN_08035058(u32 slot, u32 id);
extern u32  CreateEntryFromTemplate(Entry *src, u32 unused1, u32 unused2,
                                    u8 kind, u32 phase, u8 owner);

/* 0x080291B8 */
void SpawnFollowupEntry(Entry *e)
{
    if (e->kind == KIND_76 || e->phase == PHASE_46) {
        CreateEntryFromTemplate(e, 1, 0, KIND_34, PHASE_47, e->owner);
        FUN_08035058(GetActiveSlotValue(), NOTIFY_A);
    } else {
        CreateEntryFromTemplate(e, 1, 0, KIND_34, PHASE_7, e->owner);
        FUN_08035058(GetActiveSlotValue(), NOTIFY_B);
    }

    gRam020245A0 = 1;
    gRam02024344 = 0;
}
