/* Iki katmanli varlik listesinde bekleyen dugumleri serbest birakma
 * 0x080151C0-0x0801523B, 124 bayt  [ESLESTI]
 *
 * ROM'un yaptigi is:
 *   0x020230A0'daki bas isaretcisinden baslayarak +0x3C ile bagli dis
 *   listeyi geziyor.  Her dis dugumde +0x30 alanini "gecerli ROM
 *   isaretcisi mi" diye suzuyor.  Suzgeci gecen dugum ayni zamanda ic
 *   listenin BASI oluyor: ic liste +0x44 ile bagli ve ilk elemani dis
 *   dugumun kendisi.  Ic listede +0x27 bayti 1 olan her dugum icin:
 *     UnlinkToFree(node->unk18); node->unk18 = 0;
 *     kind &= node->flags20;   (kind burada 1, yani bit 0 testi)
 *     kind ve node->unk34 doluysa FUN_08013308(node->unk34);
 *     FUN_0801362c(node->unk38);
 *     node->kind27 = 0;
 *
 * ROM'DAN OLCULEN AYRINTILAR
 * --------------------------
 * (1) SIFIR KONTROLU ACIK DAL OLMALI -- ESLESMEYI ACAN TEK OLCUM.
 *     ROM once `cmp r1,#0 / beq`, sonra araligi suzuyor.  Aritmetik test
 *     sifiri zaten elerdi (0 - 0x08000000 = 0xF8000000 > 0xFFFFFF), ve
 *     `if (addr != 0 && addr - ROM_BASE <= ROM_SPAN)` yazildiginda agbcc
 *     ilk dali TAMAMEN SILIYOR: 120 bayt, dort eksik.  Iki kosulu ayri
 *     `if (...) goto next;` deyimlerine bolmek dali geri getirdi.
 *     Yan etki: ayni degisiklik sifir sabitinin kopyalandigi yazmaci da
 *     r1'den ROM'un r0'ina cevirdi (o iki komut da tek basina farkliydi).
 * (2) 0xF8000000 havuzdan degil `movs r0,#248 / lsls r0,#24` ile
 *     kuruluyor; bu yuzden kosul `addr - ROM_BASE` biciminde yazildi.
 *     Ust sinir (0x00FFFFFF) ise havuzdan okunuyor.
 * (3) Iki dongu de GIRIS KORUMASI + alttan donen bicim.  Kural 49
 *     uyarisi geregi kardes SelectEntityHandler'in (0x0801528C) dongu
 *     bicimi KOPYALANMADI; bicim ROM disassembly'sinden okundu.
 *     Fonksiyonun sonunda seyrek govde yok, `goto next` yalnizca dis
 *     dongunun adim noktasina gidiyor.
 * (4) `movs r0,#0 / mov r8,r0` ic dongunun PREHEADER'inda duruyor:
 *     agbcc iki sifir sabitini (unk18 ve kind27 yazimlari) dongu
 *     degismezi olarak disari tasiyor.  r8 kullanildigi icin prologda
 *     `mov r7,r8 / push {r7}` var.  Kaynakta ayri bir `zero` yereli
 *     YOK; iki duz 0 sabiti yeter.
 * (5) `ldrb r4 / cmp #1 / ... / ands r4,r0` -- AND sonucu ayni yazmacta
 *     kaliyor, yani kural 33 biciminde bilesik atama (`kind &= ...`).
 *     `if (kind & node->flags20)` yazimi denenmedi; bilesik atama zaten
 *     ROM'u verdi.
 * (6) +0x27 ofseti ldrb/strb immediate sinirini (#31) astigi icin agbcc
 *     adresi kendiliginden ayri yazmaca (r7) aliyor; kaynakta isaretci
 *     yereli yazmaya gerek YOK.
 *
 * DENENIP ELENEN YAZIM
 * --------------------
 * - `if (addr != 0 && addr - ROM_BASE <= ROM_SPAN) { ... }`: 120 bayt,
 *   iki kosul birlesiyor ve sifir dali kayboluyor (yukarida (1)).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/level_step_b1.c
 */

#include "gba_types.h"
#include "sprite_pool.h"

void UnlinkToFree(Node *node);
void FUN_08013308(void *arg);
void FUN_0801362c(void *arg);

typedef struct Entry {
    u8            pad00[0x18];
    Node         *unk18;        /* +0x18 UnlinkToFree'ye giden dugum */
    u8            pad1C[4];
    u16           flags20;      /* +0x20 */
    u8            pad22[5];
    u8            kind27;       /* +0x27 */
    u8            pad28[8];
    u32           unk30;        /* +0x30 ROM isaretcisi olarak suzuluyor */
    void         *unk34;        /* +0x34 */
    void         *unk38;        /* +0x38 */
    struct Entry *next3c;       /* +0x3C dis liste baglantisi */
    u8            pad40[4];
    struct Entry *next44;       /* +0x44 ic liste baglantisi */
} Entry;

/* +0x30 alaninin gecerli sayildigi aralik: 0x08000000-0x08FFFFFF. */
#define ROM_BASE 0x08000000
#define ROM_SPAN 0x00FFFFFF

/* 0x020230A0: dis listenin bas isaretcisi.  data/ram_map.csv'ye sembol
   EKLEME yetkim olmadigi icin sabit cast yazildi; ofset 0 oldugu icin
   kural 1'in katlama sorunu burada olusmuyor (ROM da adresi havuzdan tek
   parca okuyup `ldr r6,[r0,#0]` yapiyor). */
#define gListHead020230A0 (*(Entry **)0x020230A0)

/* 0x080151C0 */
void FUN_080151c0(void)
{
    Entry *outer;
    Entry *node;
    u32 addr;
    u32 kind;

    outer = gListHead020230A0;
    while (outer != 0) {
        addr = outer->unk30;
        if (addr == 0)
            goto next;
        if (addr - ROM_BASE > ROM_SPAN)
            goto next;

        node = outer;
        while (node != 0) {
            kind = node->kind27;
            if (kind == 1) {
                UnlinkToFree(node->unk18);
                node->unk18 = 0;
                kind &= node->flags20;
                if (kind != 0 && node->unk34 != 0)
                    FUN_08013308(node->unk34);
                FUN_0801362c(node->unk38);
                node->kind27 = 0;
            }
            node = node->next44;
        }
    next:
        outer = outer->next3c;
    }
}
