/* gEntriesA'da sahip + durum aramasi — 0x08028F4C-0x08028F97
 *
 * Once FUN_0803c400 ile parametreyi dogrular; sonuc 0 ise hemen 0 doner.
 * Gecerliyse gEntriesA'daki 15 girise (stride 148) bakar ve su ucluyu arar:
 * giris aktif (+0x00 sifir degil), +0x84 alani parametreye esit, +0x90 alani
 * 51 ya da 52. Bulursa 1, bulamazsa 0 doner.
 *
 * ROM'daki `subs #51 / cmp #1 / bls` tek bir aralik sinamasidir; agbcc bunu
 * `x == 51 || x == 52` yazimindan kendi uretiyor (aralik kanonikleştirmesi).
 *
 * `pop {r4}; pop {r1}; bx r1` — r0 canli kaldigi icin donus adresi r1'e
 * aliniyor, yani fonksiyon DEGER donduruyor (kural 35'in tersi).
 *
 * BYTE-MATCHING (76/76). Uc sey birlikte gerekti; her biri olculdu:
 *
 * 1) SAYAC `s32 i` ILE DIZI INDEKSLI `for` (kural 9/31/42). ROM disaridan
 *    bakinca saf isaretci yurumesi (`adds #148` + `cmp r1,r3`) gibi duruyor,
 *    ama dal ISARETLI (`ble`). Kardes kind_scan.c'nin elle yazilmis isaretci
 *    dongusu `bls` uretiyor. Isaretli dal ancak `i <= 14` karsilastirmasindan
 *    geliyor: agbcc guclendirme sonrasi biv'i eleyip testi giv uzerine
 *    tasirken karsilastirmanin isaretliligini KORUYOR.
 *
 * 2) +0x84 VE +0x90 ICIN TEK ISARETCI (EntryTail). Alanlari dogrudan
 *    `gEntriesA[i].unk84` / `gEntriesA[i].unk90` diye yazmak agbcc'ye UC ayri
 *    giv kurduruyor (base+0, base+132, base+144) ve `push {r4,r5,r6,lr}`
 *    veriyordu: 84 bayt / 63 fark. Iki alani tek alt-yapida toplayip
 *    `tail = &base[i].tail` demek ROM'un iki giv'ini (r1 = +0, r2 = +0x84)
 *    ve `ldr [r2,#0]` / `ldr [r2,#12]` erisimlerini uretti: 76 bayt / 46 fark.
 *
 * 3) `tail` ATAMASI `if`IN DISINDA. `if (active)` icine yazilinca giv her
 *    turda calismadigi icin agbcc guclendirmiyor, adresi dongu icinde
 *    `r3 + r5` diye yeniden kuruyordu. Kosulun onune alinca giv oldu.
 *
 * 4) `base` AYRI YEREL (kural 1/37). `gEntriesA[i]` dogrudan yazilinca agbcc
 *    ikinci giv'in baslangicini `.word gEntriesA+0x84` olarak KATLIYOR ve
 *    tabani `subs #132` ile geri hesapliyordu (72 bayt, 16 komut farki).
 *    `base = gEntriesA;` ara yereli tabani register'da tutuyor; iki giv de
 *    ondan tureyince ROM'un `ldr r0,=base / adds r2,r0,#0 / adds r2,#132 /
 *    adds r1,r0,#0` dizisi cikiyor.
 *
 * Kardesleri: src/world/kind_scan.c (0x08028E3C, isaretsiz isaretci
 * dongusu) ve src/world/entries_a4.c (0x08029244, guclendirilmemis
 * `i * 148` carpimi). Ayni tablonun uc farkli derleme bicimi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_a2.c
 */

#include "gba_types.h"

#define ENTRY_COUNT_MAX  14         /* dongu 0..14, yani 15 tur */
#define STATE_LOW        51
#define STATE_HIGH       52

typedef struct EntryTail {
    u32 owner;                      /* Entry +0x84 */
    u8  pad04[8];
    u32 state;                      /* Entry +0x90 */
} EntryTail;

typedef struct Entry {
    u8        active;               /* +0x00 */
    u8        pad01[131];
    EntryTail tail;                 /* +0x84, stride 148 */
} Entry;

extern Entry gEntriesA[];

extern u32 FUN_0803c400(u32 arg);

/* 0x08028F4C */
u32 FindEntryByOwnerState(u32 arg)
{
    Entry *base;
    EntryTail *tail;
    s32 i;

    if (FUN_0803c400(arg) == 0)
        return 0;

    base = gEntriesA;
    for (i = 0; i <= ENTRY_COUNT_MAX; i++) {
        tail = &base[i].tail;
        if (base[i].active != 0) {
            if (tail->owner == arg
             && (tail->state == STATE_LOW || tail->state == STATE_HIGH))
                return 1;
        }
    }

    return 0;
}
