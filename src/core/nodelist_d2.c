/* Bolgenin iki kimlik dizisini bosaltma — 0x08052CF8-0x08052DDB (228 bayt)
 *
 * Bolge nesnesinin +0x28 alanindaki tanim blogu iki u16 kimlik dizisini ve
 * bir yuva blogunu tarifliyor:
 *
 *     +0x05  u8   listA girdi sayisi
 *     +0x06  u8   listB girdi sayisi
 *     +0x0A  u16  listC yuva sayisi
 *
 * Bolgenin +0x0B baytindaki ISARETLI 4 bitlik ust alan ("level") sifirsa
 * fonksiyon hicbir sey yapmadan donuyor. Aksi halde:
 *
 *   1) listB'deki (+0x34) her kimlik icin RefreshThenNotify cagriliyor.
 *   2) listA'daki (+0x30) her kimlik icin dugum aranıyor. Dugum bulunduysa
 *      ve kirliyse (bit 0) ve seviyesi 1 ise, dugumun yuva dizisindeki her
 *      kimlik icin ReleaseAreaNode cagrilip yuvalar bos kimlikle doldurularak
 *      kirli biti temizleniyor. Dugum bulunduysa HER DURUMDA liste basi +
 *      kimlik ile FUN_08055D90 cagriliyor.
 *   3) Iki dizi de bos kimlikle dolduruluyor, yuva blogu DMA ile siliniyor.
 *
 * Bu, 0x08055C04'un (src/core/nodelist_c2.c) tek dugumluk halinin dizi
 * uzerinde tekrarlanmis surumu; +0x0B bitfield'lari ve ic dongu oradaki
 * ile birebir ayni kodu uretiyor.
 *
 * OLCULEN IKI AYRINTI (ikisi de tek basina denendi, ikisi de belirleyici):
 *
 * 1) IKI DIS DONGU AYNI SAYACI PAYLASIYOR. Ayri `i` ve `j` yazildiginda
 *    agbcc iki gezinme isaretcisini r4'te birlestirip sayaclari r5/r6'ya
 *    itiyordu (22/228 bayt fark); ROM ise tersini yapiyor: her iki
 *    dongude sayac r4, isaretciler r5 ve r6. Sebep dagitim onceligi
 *    (docs/COMPILER.md formulu): ayri sayaclar 8 refs / 26 omur ->
 *    0.923 verirken, paylasilan sayac 16 refs / 54 omur -> 1.185
 *    veriyor ve isaretcinin onune geciyor. floor_log2(16)=4 carpani
 *    referanslari birlestirmeyi karli kiliyor. Fark 22 -> 8 bayta indi.
 *
 * 2) BILDIRIM SIRASI, DAGITIM ESITLIGINI BOZUYOR. Kalan 8 bayt, ikinci
 *    dongude sayacin mi isaretcinin mi `sl`ye (yuksek register) kaydedilip
 *    digerinin yigina tasacagi farkiydi. `-dg` dokumunde iki artirim
 *    gecicisi de refs=4 / omur=48 tasiyor, yani ONCELIKLERI ESIT; GCC 2.8
 *    esitligi pseudo NUMARASIYLA bozuyor ve pseudo numaralari kaynaktaki
 *    BILDIRIM SIRASINDAN geliyor (area=22, desc=23, node=24, ...).
 *    Sayac `entryA`dan SONRA bildirilince isaretcinin gecicisi kucuk
 *    numarayi alip `sl`yi kapiyordu. `i`yi isaretcilerden ONCE bildirmek
 *    sirayi cevirdi: `mov sl, r4` (sayac) + `str r6, [sp]` (isaretci).
 *    Fark 8 -> 0.
 *
 *    Bu, docs/COMPILER.md'deki "bildirim sirasi yigin yerlesimini
 *    degistirmez" olcumunun tamamlayicisi: yigin yerlesimini degistirmiyor
 *    ama ESIT ONCELIKLI pseudo'lar arasindaki dagitim sirasini belirliyor.
 *
 * DENENIP ELENENLER:
 *   - `continue` yerine `if (node != 0) { ... }` bloku: tek bayt
 *     degistirmedi (ayni RTL).
 *   - Artirimlari `for` yan tumcesinden govdeye, cagrinin hemen ardina
 *     tasimak: dongu dondurmesini bozdu, cok daha kotu kod.
 *   - Yalniz `id`/`k` bildirimlerini oynatmak: sayac hala isaretcilerden
 *     sonra kaldigi icin etkisiz.
 *
 * Diger uygulanan kurallar:
 *   - Kural 19: `desc` okumasi seviye kontrolunden ONCE yazildi; ROM da
 *     `ldr r7, [r0, #40]`i maske testinden once yayiyor.
 *   - Kural 24/26: ust nibble ISARETLI bitfield; `== 0` testi agbcc'de
 *     kaydirmasiz `movs #240 / ands / cmp #0` uretiyor.
 *   - Kural 37: iki dizi gezicisi ayri yerel.
 *   - Kural 43: sayac ve gezici `for` artiriminda, ROM sirasiyla (once
 *     `adds r4, #1`, sonra `adds r6, #2`).
 *   - Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *   - Sondaki uc cagri gezicileri degil yeniden `area->...` alanlarini
 *     okuyor; ROM da oyle.
 *
 * ESLESME: 228/228 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_d2.c
 */

#include "gba_types.h"

#define LEVEL_BASE  1

typedef struct SlotDesc {
    u8  pad00[5];
    u8  countA;                 /* +0x05 */
    u8  countB;                 /* +0x06 */
    u8  pad07[3];
    u16 countC;                 /* +0x0A */
} SlotDesc;

typedef struct Node {
    struct Node *next;          /* +0x00 */
    u8           pad04[4];
    u16          id;            /* +0x08 */
    u8           pad0A;
    u8           dirty : 1;     /* +0x0B bit 0    */
    u8           pad0B : 3;     /* +0x0B bit 1..3 */
    s8           level : 4;     /* +0x0B bit 4..7 */
    u8           pad0C[8];
    SlotDesc    *desc;          /* +0x14 */
    u16         *slots;         /* +0x18 */
} Node;

typedef struct Area {
    u8        pad00[11];
    u8        pad0B  : 4;       /* +0x0B bit 0..3 */
    s8        level  : 4;       /* +0x0B bit 4..7 */
    u8        pad0C[28];
    SlotDesc *desc;             /* +0x28 */
    u8        pad2C[4];
    u16      *listA;            /* +0x30 */
    u16      *listB;            /* +0x34 */
    void     *listC;            /* +0x38 */
} Area;

extern Node *gNodeListHead;         /* 0x02035A70 */

extern Node *FindNode(u32 id);
extern void  RefreshThenNotify(u32 id);
extern void  ReleaseAreaNode(u32 a, u32 b);
extern void  FillSlotsWithNone(void *dest, u32 count);
extern void  ClearSlots(void *dest, u32 count);
extern void  FUN_08055d90(u32 *head, u32 id);

/* 0x08052CF8 */
void ClearAreaIdArrays(Area *area)
{
    SlotDesc *desc;
    Node     *node;
    int       i;                /* iki dis dongude de ayni sayac */
    u16      *entryB;
    u16      *entryA;
    u16      *slot;
    u32       id;
    int       k;

    desc = area->desc;
    if (area->level == 0)
        return;

    entryB = area->listB;
    for (i = 0; i < desc->countB; i++, entryB++)
        RefreshThenNotify(*entryB);

    entryA = area->listA;
    for (i = 0; i < desc->countA; i++, entryA++) {
        id = *entryA;
        node = FindNode(id);
        if (node == 0)
            continue;

        if (node->dirty) {
            if (node->level == LEVEL_BASE) {
                slot = node->slots;
                for (k = 0; k < node->desc->countB; k++, slot++)
                    ReleaseAreaNode(*slot, 0);
                FillSlotsWithNone(node->slots, node->desc->countB);
                node->dirty = 0;
            }
        }

        FUN_08055d90((u32 *)&gNodeListHead, id);
    }

    FillSlotsWithNone(area->listB, desc->countB);
    FillSlotsWithNone(area->listA, desc->countA);
    ClearSlots(area->listC, desc->countC);
}
