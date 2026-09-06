/* Kimligi listede bulup sayacini artirma, yoksa yedek dugumu kurma.
 * 0x080543D0, 126 bayt.
 *
 * Liste basligi +0x00 bas, +0x04 yedek dugum. Liste kimlige gore SIRALI:
 * arama, gecerli kimlik arananı GECINCE duruyor.
 *
 *   bulunursa  -> dugumun +0x0B alanindaki UST DORTLU bir artirilip
 *                 dugum donduruluyor (basvuru sayaci gibi davraniyor)
 *   bulunmazsa -> yedek dugumun kimligi 0x7FEF (bos isareti) ise
 *                 listeden cikarilip yeni kimlikle kurulup geri takiliyor
 *   yedek bos degilse -> 0
 *
 * Ust dortlu `lsls #24 / asrs #28` ile ISARETLI okunuyor, yani alan s8 ve
 * ust dortlu isaretli bir sayac.
 *
 * Bayrak kurulumundaki iki maske ROM'da -2 ve -13 olarak kuruluyor
 * (`movs #2 / negs` sonra `subs #11`); ikinci sabit birincisinden
 * turetiliyor, agbcc'nin ardisik sabit kalibi. Kaynakta `& ~1` ve `& ~12`.
 *
 * Kardes dosyalar: nodelist_c3.c bu fonksiyonun imzasini zaten
 * bildiriyordu (`Node *FUN_080543d0(Node **head, s32 index)`), nodelist_b6.c
 * de cagiriyor. Node yerlesimi oradan alindi.
 *
 * DURUM: PARK, 126/126 boyut TUTUYOR, fark 56.
 *
 * Sifirdan 56'ya dort adimda gelindi ve ucu ayni mekanizmaydi (kural 49):
 *   1. `kind` alani u8 (ROM `ldrb` + `lsls #24 / asrs #28`; s8 yapinca
 *      derleyici `ldrsb` + `asrs #4`e katliyor)
 *   2. dongu ROM bicimine: giris korumasi + acik atlamali do/while
 *      (`break` yazinca agbcc donguyu donduruyor)          120 -> 128
 *   3. "bulundu" govdesi fonksiyonun SONUNA                128 -> fark 87
 *   4. `return 0` da sona                                  fark 87 -> 56
 *
 * Kalan 56 yazmac dagitimi: ROM r6=liste r5=kimlik, bizde tersi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_a1.c
 */

#include "gba_types.h"

#define SPARE_ID 0x7FEF

typedef struct Node {
    struct Node *next;          /* 0x00 */
    u8    pad04[4];
    u16   id;                   /* 0x08 */
    u8    slot;                 /* 0x0A */
    s8    kind;                 /* 0x0B */
} Node;

typedef struct NodeList {
    Node *head;                 /* 0x00 */
    Node *spare;                /* 0x04 */
} NodeList;

extern void ListRemove(NodeList *list, Node *node);
extern void InsertSorted(NodeList *list, Node *node, s32 id);

/* 0x080543D0 */
Node *FUN_080543d0(NodeList *list, s32 id)
{
    Node *cur;
    Node *spare;
    s32 k;
    s32 cid;
    s32 hi;
    s32 k2;

    cur = list->head;
    spare = list->spare;
    if (cur == 0) goto insert;

scan:
    cid = cur->id;
    if (cid == id) goto found;
    if (cid > id) goto insert;
    cur = cur->next;
    if (cur != 0) goto scan;

insert:
    if (spare->id != SPARE_ID) goto none;

    ListRemove(list, spare);
    spare->id = id;
    k = (spare->kind & 15) | 16;
    spare->slot = 0;
    k = k | 2;
    k = k & ~1;
    k = k & ~12;
    spare->kind = k;
    InsertSorted(list, spare, id);
    return spare;

    /* ROM bu govdeyi fonksiyonun SONUNDA tutuyor (`beq` ileri atliyor);
     * dongunun icine yazmak blogu one aliyor ve dallanma tersine donuyor. */
found:
    /* AYRI yerel: `k` hem insert hem found dalinda kullanilinca 19 referansa
     * cikip r2'ye dusuyordu; bolununce 12'ye inip ROM'un r0'ini aliyor. */
    k2 = cur->kind;
    hi = (s32)(k2 << 24) >> 28;
    cur->kind = ((hi + 1) << 4) | (k2 & 15);   /* ROM once (hi+1)<<4 kuruyor */
    return cur;

none:
    return 0;
}
