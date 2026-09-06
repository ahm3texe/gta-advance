/* Giris zamanlayicisini ilerletme -- 0x08028D44-0x08028DC3
 *
 * ESLESTI: 128/128 bayt, fark 0.  make c-match TEMIZ, make c-review TEMIZ.
 *
 * Indisten 148 baytlik giris hesaplayip iki asamali ROM tablosundan
 * hedefi cozuyor; giris uygun durumdaysa ve +0x90 alani 3 ise isaretci
 * kurup FUN_08013CFC'yi cagiriyor, zamanlayiciyi 4 azaltiyor ve pozitif
 * kalirsa 1 donuyor.
 *
 * PARAMETRE TIPLERI komut dizisinden okundu: ikinci parametre `lsls #16 /
 * asrs #16` ile ISARETLI 16 bit (s16), ucuncusu `lsls #24 / lsrs #24` ile
 * ISARETSIZ 8 bit (u8). Genis tip yazmak bu kirpma komutlarini goturur.
 *
 * Zamanlayici sinamasi `lsls #16` + `cmp <= 0`, yani azaltilmis yarim soz
 * ISARETLI olarak sinaniyor.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 donus degeri tasiyor, imza u32.
 *
 * TIP BIRLESTIRILDI: gRam020246F0 zaten src/world/table_entries.c'de
 * `Entry[20]` olarak tanimliymis ve o tanimda +0x02 (unk02) ile +0x04
 * (unk04) DOGRU yerdeymis. Elle `base + index*148` hesaplamak yerine
 * `&gRam020246F0[index]` kullanmak dogru olan; ayni sembole iki tur
 * vermek TYPES-001 kapisini kiriyordu. Eksik alanlar (mark +0x2A,
 * state +0x2B, tableIndex +0x64, phase +0x90) dolgudan oyuldu ve
 * table_entries.c 3/3 KORUNDU.
 *
 * ------------------------------------------------------------------
 * SON 4 BAYT NASIL KAPANDI (124 -> 128, fark 55 -> 0)
 * ------------------------------------------------------------------
 * Komut komut diff, TEK bir bolgenin (mark yazimi ile bl arasi) saptigini
 * gosterdi. ROM ile bizim eski cikti:
 *
 *   ROM                         eski bizim
 *   adds r0, r4, #0             adds r0, r4, #4     <- arg1 ONCE
 *   adds r0, #140               adds r1, #98        <- r1 = entry+0x2A idi
 *   ldr  r0, [r0, #0]           ldr  r1, [r1, #0]
 *   asrs r0, r0, #2             lsrs r1, r1, #2
 *   ldr  r1, [r3, #4]           ldr  r2, [r3, #4]
 *   lsls r0, r0, #2             lsls r1, r1, #2
 *   adds r0, r0, r1             adds r1, r1, r2
 *   ldr  r1, [r0, #0]           ldr  r1, [r1, #0]
 *   adds r0, r4, #4             (yok -- yukarida yapilmisti)
 *
 * MEKANIZMA: arg2 zinciri cagri kurulumu icinde uretilince arg1 (`&entry->
 * unk04`) ONCE r0'a girdi, arg2 zincirine r1 kaldi; r1 o anda zaten
 * `entry+0x2A` (mark isaretcisi) tasidigi icin derleyici `entry+0x8C`yi
 * `adds r1, #98` ile TEK komutta uretti. Iste eksik olan 2 bayt buydu --
 * geri kalan 2 bayt da bunun sonucu: kod 2 bayt kisalinca havuz oncesi
 * hizalama `movs r0,r0` (nop) dolgusu dusuyordu.
 *
 * COZUM: arg2'yi AYRI BIR DEYIME al (`src = ...;`). Boylece RTL sirasi
 * ROM'unki gibi olur: once arg2 zinciri (r0 kazikta, o anda 0xFF olu),
 * sonra cagri kurulumunda `adds r0, r4, #4`. r0 o noktada adres tasimadigi
 * icin `entry+0x8C` r4'ten YENIDEN hesaplaniyor -> aranan fazladan komut.
 *
 * Ikinci parca: unk8C alani s32 (ISARETLI). `>> 2` boylece `asrs` uretiyor.
 *
 * OLCULEN KATKILAR (ayri ayri denendi):
 *   temel (u32 unk8C, arg2 cagri icinde)   -> 124 bayt, fark 55
 *   YALNIZ s32 unk8C                       -> 124 bayt, fark 54  (yetmez)
 *   YALNIZ ayri `src` yereli               -> 128 bayt, fark  1  (asrs eksik)
 *   IKISI BIRDEN                           -> 128 bayt, fark  0  ESLESTI
 * Yani boyutu duzelten deyim ayirmasi, kalan tek bayti duzelten s32.
 *
 * ELENEN YOLLAR (onceki elle-hesaplamali surumden; TEKRAR DENEMEYIN):
 *   kaydirmada (s32) cast                -> 124 bayt
 *   ilk argumani (u8*)entry+4 yapmak     -> 124 bayt
 *   ilk argumani entry->pad04 yapmak     -> 124 bayt
 *   ilk argumani &entry->unk02 + 1       -> 124 bayt
 * Bunlarin hicbiri ise yaramadi cunku sorun ARG1'IN BICIMI DEGIL, arg2'nin
 * NE ZAMAN uretildigiydi. Arg1 ifadesini kurcalamak yanlis eksendi.
 * DERS: "yanlis yazmac" gibi gorunen fark, aslinda YAYILIM SIRASI farkiydi;
 * bir alt ifadeyi ayri deyime almak (kural 40'in tersi yonde kullanimi)
 * cagri argumanlarinin uretim sirasini ROM'unkine cevirir.
 *
 * Havuz notu (kayit icin): havuz kelimesi hic eksik degildi. Iki havuz da
 * ayni iki sabiti tasiyordu (0x020246F0 ve 0x08BD3448), sadece 4 bayt
 * kaymislardi; eksik olan havuzdan ONCEKI koddu.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/step_entry_timer.c
 */

#include "gba_types.h"

#define STATE_READY  1
#define PHASE_DONE   3
#define TIMER_STEP   4
#define MARK_VALUE   0xFF

#define ROM_TABLE ((TableA *)0x08BD3448)

typedef struct TableB {
    u8    pad00[4];
    u32 **slots;                /* +0x04 */
} TableB;

typedef struct TableA {
    u8       pad00[4];
    TableB **slots;             /* +0x04 */
} TableA;

typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01;
    u16 unk02;                  /* +0x02 */
    u32 unk04;                  /* +0x04 (serbest birakilacak blok) */
    u8  pad08[0x22];
    u8  mark;                   /* +0x2A */
    u8  state;                  /* +0x2B */
    u8  pad2C[0x38];
    u8  tableIndex;             /* +0x64 */
    u8  pad65[0x27];
    s32 unk8C;                  /* +0x8C */
    u32 phase;                  /* +0x90 */
} Entry;

extern Entry gRam020246F0[20];

extern void FUN_08013cfc(void *dest, u32 *src, u32 arg);

/* 0x08028D44 */
u32 StepEntryTimer(u32 unused, s16 index, u8 arg)
{
    Entry *entry;
    TableB *b;
    u32 *target;
    u32 *src;
    u32 phase;

    entry = &gRam020246F0[index];
    b = ROM_TABLE->slots[entry->tableIndex];
    phase = entry->phase;
    target = (u32 *)b->slots[phase];

    if (entry->state != STATE_READY)
        return 0;
    if (phase != PHASE_DONE)
        return 1;

    entry->mark = MARK_VALUE;
    src = (u32 *)((TableB *)target)->slots[entry->unk8C >> 2];
    FUN_08013cfc(&entry->unk04, src, arg);

    entry->unk02 -= TIMER_STEP;
    if ((s16)entry->unk02 <= 0)
        return 0;
    return 1;
}
