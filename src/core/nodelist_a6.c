/* Dugumu sifirlayip yuva dizilerini tanimdan doldurma — 0x08053930 (196 bayt)
 *
 * ROM'dan OKUNAN akis:
 *   1. gRam02030C00 = 0  (maske kapisi anahtari; nodelist_c3.c ve
 *      nodelist_d5.c ile ayni sembol, orada `u32` bildirilmis).
 *   2. IME sakla/kapat, DMA3 ile dugumun 32 baytini (8 kelime) SIFIRLA,
 *      DMA3.control'u olu oku, IME geri yaz. Kaynak yigindaki sifir
 *      yereli, denetim sozcugu 0x85000008 (enable | 32-bit | src fixed).
 *   3. desc->countB kadar ardisik bos yuva ara (FindFreeSlotRun, 0x08055D18)
 *      -> node->slotsB; countB sifir degilse ve donen isaretci NULL ise
 *      fonksiyon biter.
 *   4. Ayni sey countA / node->slotsA icin.
 *   5. Iki kopyalama dongusu: slotsB <- desc->listB (+0x0C), her yazilan
 *      kimlikle FUN_08053650; slotsA <- desc->listA (+0x08), her yazilan
 *      kimlikle LinkAreaEntryIfEligible (0x08052C68).
 *   6. node->desc = desc (+0x14), node->unk0A = 0xFF (+0x0A).
 *
 * OLCULEN 1 — dugum 32 bayt: DMA denetim sozcugunun alt yarisi 8 kelime,
 *   yani 0x20 bayt. Struct'in son alani +0x1C (slotsB) oldugu icin bu
 *   nodelist_d3.c'deki yerlesimle tutarli; +0x0A alani orada `pad0A` idi,
 *   burada 0xFF yaziliyor, `unk0A` olarak birakildi (anlami bilinmiyor,
 *   ad uydurulmadi).
 *
 * OLCULEN 2 — sifir sabiti UC yerde paylasiliyor: ROM tek `movs r0,#0`
 *   uretip onu gRam02030C00 store'unda, `REG_IME = 0`da ve yigindaki
 *   doldurma yerelinde kullaniyor. Bunun cikmasi icin `fill = 0;` satiri
 *   `REG_IME = 0;`den SONRA yazilmali (src/ui/menu_screen.c'deki VRAM
 *   temizleme blogunda da ayni sira).
 *
 * OLCULEN 3 — NULL kontrolu DONEN DEGERI test ediyor, alani degil:
 *   ROM `adds r1,r0,#0` ile donusu ayri bir yazmaca kopyalayip `str`den
 *   sonra `cmp r1,#0` yapiyor; `node->slotsB` yeniden okunmuyor. Bu yuzden
 *   cagri sonucu once `slots` yereline aliniyor, alana ondan yaziliyor.
 *   Sayac testi ise ALANDAN yeniden okunuyor (`ldrb r0,[r7,#5]`), cunku
 *   arada cagri var. Kural 34: iki kosul erken `return` olarak yazildi.
 *
 * OLCULEN 4 — iki test tek maskeye KATLANMIYOR: `countB != 0 && slots == 0`
 *   iki FARKLI degisken uzerinde oldugu icin fold_truthop'a giremiyor;
 *   ROM'daki `cmp #0/beq` + `cmp #0/beq` cifti dogrudan cikiyor.
 *   (nodelist_d3.c'deki OLCULEN 1 ile karsilastir: orada ayni bayta iki
 *    test vardi ve yazim bicimi katlamayi belirliyordu.)
 *
 * OLCULEN 5 — BELIRLEYICI OLAN: her donguye KENDI kaynak isaretcisi.
 *   Iki dongu tek `src` yerelini paylasinca kalan tek fark 14 bayttir ve
 *   TAMAMI r5/r6 takasidir: ROM sayaci r5'e, kaynagi r6'ya koyar; paylasilan
 *   `src` ile tam tersi cikar. `srcB` ve `srcA` ayri yereller yapilinca
 *   kaynagin omru ikiye bolunuyor, oncelikte sayacin altina dusuyor ve
 *   dagitim ROM'unkine donuyor: 14 -> 0.
 *   DIKKAT — bu, nodelist_d3.c'deki OLCULEN 3'un TERSI degil ama ayni
 *   mekanizmanin baska yuzu: orada omur BOLMEK gerekiyordu, burada da.
 *   Hedef isaretcisini (`dst`) bolmek ETKISIZ; sayaci bolmek de ETKISIZ.
 *   Bolunmesi gereken tam olarak KAYNAK isaretcisi.
 *
 * DENENIP ELENENLER (hepsi olculdu, boyut her zaman 196):
 *   - Bildirim sirasinin 720 permutasyonunun TAMAMI: fark hep 14.
 *     Bildirim sirasi bu fonksiyonda dagitimi hic degistirmiyor
 *     (docs/COMPILER.md'deki "yiginda bildirim sirasi belirleyici degil"
 *     gozlemi yazmac dagitimi icin de gecerli cikti).
 *   - Iki donguye ayri sayac (`i` / `j`): 14. Etkisiz.
 *   - Sayaci her donguye blok-yerel yapmak: 14. Etkisiz.
 *   - Iki donguye ayri HEDEF isaretcisi (`dst` / `dst2`): 14. Etkisiz.
 *   - `i = 0;` dongulerden once, `for (; ...)` bicimi: 26 (kotulesti).
 *   - Kaynak atamasini hedef atamasindan once yazmak: 24 (kotulesti).
 *   - Artirim sirasini `i++, src++, dst++` yapmak: 16. ROM sirasi
 *     `i++, dst++, src++` (kural 43).
 *
 * Kural 43: sayac ve IKI isaretci birlikte ilerliyor -> ucu de `for`
 *   artiriminda, ROM sirasiyla (`adds r5,#1` / `adds r4,#2` / `adds r6,#2`).
 * Kural 9/31: sayac `int`; ROM `bge`/`blt` (isaretli) uretiyor, u8 alan
 *   karsilastirmada `int`e yukseliyor.
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * Kural 1: 0x02030C00 extern sembol (gRam02030C00), sabit cast degil.
 *
 * `*dst = *src; FUN_08053650(*dst);` — cagri argumani KAYNAKTAN degil
 * HEDEFTEN yeniden okunuyor (`strh r0,[r4]` hemen ardindan `ldrh r0,[r4]`).
 * `FUN_08053650(*src)` yazmak bu ikinci `ldrh`yi eler.
 *
 * ESLESME: 196/196 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_a6.c
 */

#include "gba_types.h"
#include "gba_io.h"

/* enable | 32-bit birim | kaynak sabit | 8 kelime = 32 bayt */
#define DMA_CLEAR_NODE  0x85000008

#define NODE_UNK0A_INIT 0xFF

/* nodelist_d3.c ve nodelist_d6.c'deki SlotDesc ile ayni taban; buradan
 * ayrica +0x08 ve +0x0C isaretcileri okunuyor. */
typedef struct SlotDesc {
    u8   pad00[4];
    u8   countA;                /* +0x04 birincil dizinin uzunlugu */
    u8   countB;                /* +0x05 ikincil dizinin uzunlugu  */
    u8   pad06[2];
    u16 *listA;                 /* +0x08 birincil kimlik kaynagi   */
    u16 *listB;                 /* +0x0C ikincil kimlik kaynagi    */
} SlotDesc;

/* nodelist_d3.c'deki Node ile ayni yerlesim; toplam 32 bayt (DMA sifirlama
 * bunu olcuyor). */
typedef struct Node {
    u8        pad00[10];
    u8        unk0A;            /* +0x0A */
    u8        pad0B[9];
    SlotDesc *desc;             /* +0x14 */
    u16      *slotsA;           /* +0x18 */
    u16      *slotsB;           /* +0x1C */
} Node;

extern u32 gRam02030C00;

extern u16 *FindFreeSlotRun(int count);
extern void FUN_08053650(s32 index);
extern void LinkAreaEntryIfEligible(s32 index);

/* 0x08053930 */
void FUN_08053930(Node *node, SlotDesc *desc)
{
    u16 *dst;
    u16 *srcB;
    u16 *srcA;
    u16 *slots;
    int  i;
    u32  fill;
    u16  ime;

    gRam02030C00 = 0;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = node;
    REG_DMA3.control = DMA_CLEAR_NODE;
    REG_DMA3.control;
    REG_IME = ime;

    slots = FindFreeSlotRun(desc->countB);
    node->slotsB = slots;
    if (desc->countB != 0 && slots == 0)
        return;

    slots = FindFreeSlotRun(desc->countA);
    node->slotsA = slots;
    if (desc->countA != 0 && slots == 0)
        return;

    dst = node->slotsB;
    srcB = desc->listB;
    for (i = 0; i < desc->countB; i++, dst++, srcB++) {
        *dst = *srcB;
        FUN_08053650(*dst);
    }

    dst = node->slotsA;
    srcA = desc->listA;
    for (i = 0; i < desc->countA; i++, dst++, srcA++) {
        *dst = *srcA;
        LinkAreaEntryIfEligible(*dst);
    }

    node->desc = desc;
    node->unk0A = NODE_UNK0A_INIT;
}
