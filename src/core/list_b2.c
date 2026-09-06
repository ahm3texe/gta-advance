/* Kayit tablosunu kaynak listeden doldurma — 0x0800DCB4-0x0800DD4D
 *
 * 0x02015650'deki tablo nesnesi 16x16'lik bir kayit dizisi tutuyor
 * (girdi 12 bayt, satir 192 bayt, dizi 0x24..0xC24 = 0xC00). Iki sayac
 * hemen dizinin ardinda: +0xC24 dis sayac, +0xC28 ic sayac. Fonksiyon
 * 8 baytlik kaynak kayitlar uzerinde duz yuruyup her birini tabloya
 * SUTUN SIRASIYLA yaziyor: dis sayac sutunu (i), ic sayac satiri (j)
 * tariyor, hedef adres tablo + i*12 + j*192. ROM'daki adres uretimi
 * (lsl#1 + add + lsl#2, sonra her adimda +0xC0) tam olarak budur.
 *
 * Kaynak kaydin sayisi sifirsa girdinin uc kelimesi de sifirlaniyor;
 * degilse sayi, ikinci alan ve 0x02016290 havuzunda ilerleyen isaretci
 * yaziliyor. Havuz isaretcisi her kayitta sayi kadar KELIME ilerliyor
 * (lsls #2). Sonda tablo nesnesinin ilk kelimesi sifirlaniyor.
 *
 * 0x02016290, kardes dosyada (src/core/list_b1.c) DMA ile sifirlanan
 * 18400 baytlik blogun ta kendisi; ROM onu AYRI literal olarak yukluyor,
 * 0x02015650 + 0xC40 olarak degil -- bu yuzden ayri nesne olarak yazildi
 * (kardesteki ayni gerekce).
 *
 * TABAN NEDEN extern (kural 1): ROM 0x02015650'yi havuzdan okuyup uzerine
 * 0xC24'u AYRI bir literalden ekliyor. Ham adres cast'i kullanilsaydi
 * agbcc 0x02016274'u tek literale katlar ve bu uc komut kaybolurdu.
 * Havuz (0x02016290) ofsetsiz kullanildigi icin cast makrosu yetiyor:
 * iki bicim de tek `ldr rX,[pc,...]` uretiyor.
 *
 * Kural 9/31: dort karsilastirmanin dordu de isaretli (bge/blt), bu yuzden
 * sayaclar ve iki alan s32. Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * `src->count` uc ayri yerde yeniden okunuyor (0x800DCF4, 0x800DD14,
 * 0x800DD1E): aradaki store'lar agbcc'de yuku olduruyor, bu yuzden
 * kaynakta da yerele alinmadan her seferinde alan okundu.
 *
 * DENENENLER (tekrar etme):
 *   - Parametreyi DOGRUDAN yurutmek (`void FUN_0800dcb4(SourceEntry *src)`
 *     + `src++`): 154/154 boyut, tek fark parametre kopyasinin sirasi.
 *     agbcc parametre->pseudo kopyasini prologa koyuyor, o yuzden
 *     `adds r3,r0,#0` havuz literalinden ONCE cikiyordu; ROM'da SONRA.
 *     Cozum: parametre okunur birakilip yuruyucu AYRI yerel olarak
 *     `src = entries;` deyimiyle kuruldu -- kopya artik govde deyimi,
 *     havuz atamasindan sonraki sirayi aliyor. 4 bayt -> 0. (Kural 11/22
 *     ailesi: atamanin YERI belirleyici.)
 *   - `pool`u bildirimde ilklendirmek (`u32 *pool = gPool02016290;`)
 *     tek basina hicbir sey degistirmedi; sira farki oradan gelmiyordu.
 *     Ayri yerel ile birlikte de ayni sonucu veriyor, ama okunurluk icin
 *     iki atama da govdede, ROM sirasiyla birakildi (kural 19).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/list_b2.c
 *
 * GEREKLI: data/ram_map.csv'ye 0x02015650 icin gRam02015650 kaydi
 * (3116 bayt: 0x00..0xC2C). Bu dosya kaydi kendisi eklemiyor.
 */

#include "gba_types.h"

/* Kayitlarin veri isaretcilerinin gosterdigi kelime havuzu. */
#define gPool02016290  ((u32 *)0x02016290)

/* Tablodaki tek kayit: 12 bayt. */
typedef struct {
    u32 count;                  /* 0x00 -- kaynaktaki 16 bitlik sayi */
    u32 unk04;                  /* 0x04 -- kaynaktan oldugu gibi kopyalanir */
    u32 *data;                  /* 0x08 -- havuz icindeki dilim */
} TableEntry;

/* Kaynak kayit: 8 bayt, duz dizi olarak yurunuyor. */
typedef struct {
    u16 count;                  /* 0x00 */
    u16 pad02;
    u32 unk04;                  /* 0x04 */
} SourceEntry;

/* 0x02015650'deki tablo nesnesi. */
typedef struct {
    u32 unk00;                  /* 0x00 -- sonda sifirlaniyor */
    u8  pad04[0x20];
    TableEntry rows[16][16];    /* 0x24 -- satir 192 bayt, toplam 0xC00 */
    s32 columnCount;            /* 0xC24 -- dis dongu siniri */
    s32 rowCount;               /* 0xC28 -- ic dongu siniri */
} EntryTable;

extern EntryTable gRam02015650;

/* 0x0800DCB4 */
void FUN_0800dcb4(SourceEntry *entries)
{
    u32 *pool;
    SourceEntry *src;
    s32 i;
    s32 j;

    pool = gPool02016290;
    src = entries;

    for (i = 0; i < gRam02015650.columnCount; i++) {
        for (j = 0; j < gRam02015650.rowCount; j++) {
            TableEntry *entry = &gRam02015650.rows[j][i];

            if (src->count == 0) {
                entry->count = 0;
                entry->unk04 = 0;
                entry->data = 0;
            } else {
                entry->count = src->count;
                entry->unk04 = src->unk04;
                entry->data = pool;
            }

            pool += src->count;
            src++;
        }
    }

    gRam02015650.unk00 = 0;
}
