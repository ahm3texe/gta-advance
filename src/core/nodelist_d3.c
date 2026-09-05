/* Dugumun iki yuva dizisini bosaltma — 0x080539F4-0x08053AD7  (228 bayt)
 *
 * Dugumun tanim yapisi (+0x14) iki ayri yuva sayaci tasiyor: +0x05 ikincil
 * dizinin (+0x1C), +0x04 birincil dizinin (+0x18) uzunlugu. Fonksiyon once
 * ikincil diziyi, sonra birincil diziyi geziyor, en sonda ikisini de bos
 * kimlikle dolduruyor.
 *
 *   1. Ikincil dizi: her kimlik icin FUN_08054744 ile dugum aliniyor; dugum
 *      kirli VE seviyesi 1 ise FUN_080536BC cagriliyor. Ardindan
 *      gRam02035780 listesi o kimlikle tazeleniyor.
 *   2. Birincil dizi: her kimlik icin FindNode; dugum varsa, kirliyse ve
 *      seviyesi 1 ise o dugumun KENDI yuva dizisi bosaltiliyor
 *      (src/core/nodelist_c2.c ile ayni kalip: her yuva icin FUN_08052750,
 *      sonra FillSlotsWithNone, sonra kirli biti temizle). Dugum bulunduysa
 *      gNodeListHead tazeleniyor.
 *   3. Iki dizi de FillSlotsWithNone ile bos kimlige cekiliyor.
 *
 * OLCULEN 1 — ayni bayrak baytina iki test, IKI FARKLI kod:
 *   Birinci dongude ROM tek maske uretiyor: `movs #241 / ands / cmp #17`.
 *   Ikinci donguda ayni iki testi AYRI uretiyor: `movs #1 / ands / cmp #0`,
 *   sonra `movs #240 / ands / cmp #16` — tek `ldrb` paylasilarak.
 *   Belirleyen sey kaynaktaki YAZIM bicimi: tek ifadede `&&` ile baglanan
 *   iki karsilastirmayi agbcc (GCC 2.8.1 fold_truthop) tek maskeye katliyor,
 *   ayri `if` deyimlerini katlamiyor. Bu yuzden birinci dongu `if (a && b)`,
 *   ikinci dongu ic ice `if` yazildi. src/core/nodelist_c2.c'de ikinci test
 *   `|| force` tasidigi icin zaten katlanmiyordu; buradaki birinci dongu
 *   katlamanin saf ornegi.
 *
 * OLCULEN 2 — kimlik yereli `u16` olmali, `u32` degil:
 *   `u32 id = *slotB;` ile ROM'un `ldrh r0,[r5]` + `adds r4,r0,#0` ciftini
 *   TERS uretiyorduk (`ldrh r4` + `adds r0,r4`), 2 bayt fark. `u16` yerel
 *   HImode kaldigi icin cagri argumani ayri bir SImode pseudo'ya genisliyor:
 *   yukleme arguman register'ina (r0) dusuyor, cagriyi asan kopya r4'e.
 *   Kural 15'in yerel-degisken hali. Iki dongu de `u16` yazildi; dizilerin
 *   gercek eleman tipi zaten `u16`.
 *
 * Kirli bitin temizlenmesi `movs #2 / negs` ciftini veriyor: ~1 = -2, yani
 * maske 32 bitte kaliyor (kural 26). Bitfield atamasi bunu uretiyor.
 *
 * OLCULEN 3 — dongu gecicileri BLOK KAPSAMLI olmali (kural 40):
 *   `entry` ve `id`'yi iki dongude ortak fonksiyon-kapsamli yerel yapmak
 *   omurlerini butun fonksiyona yayiyor; boylece birinci dongunun `entry`si
 *   de callee-saved istiyor, `desc` r7 yerine r8'e itiliyor ve her
 *   `desc->countX` okumasi bir `mov r2,r8` kazaniyor: 248 bayt / 230 fark.
 *   Her donguye kendi bloguna ait `id`/`entry` verilince birinci dongunun
 *   `entry`si kisa omurlu kaliyor (ROM'da r1), `desc` r7'ye oturuyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * Kural 43: sayac ve isaretci birlikte ilerliyor -> ikisi de `for`
 *   artiriminda, ROM sirasiyla (once `adds #1`, sonra `adds #2`).
 * Kural 9/31: sayaclar `int`; ROM `blt`/`bge` (isaretli) uretiyor.
 *
 * ESLESME: 228/228 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_d3.c
 */

#include "gba_types.h"
#include "node_list.h"

#define LEVEL_BASE  1

/* Yuva sayaclarini tasiyan tanim yapisi. +0x06 alani src/core/nodelist_c2.c
 * icindeki SlotDesc.count ile ayni. */
typedef struct SlotDesc {
    u8 pad00[4];
    u8 countA;                  /* +0x04 birincil dizinin uzunlugu */
    u8 countB;                  /* +0x05 ikincil dizinin uzunlugu  */
    u8 count;                   /* +0x06 dugumun kendi dizisi      */
} SlotDesc;

/* src/core/nodelist_c2.c'deki Node ile ayni yerlesim; +0x1C eklendi. */
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
    u16         *slotsA;        /* +0x18 */
    u16         *slotsB;        /* +0x1C */
} Node;

extern Node *gNodeListHead;         /* 0x02035A70 */

extern Node *FUN_08054744(u32 id);
extern Node *FindNode(u32 id);
extern void  FUN_080536bc(Node *node);
extern void  FUN_08052750(u32 a, u32 b);
extern void  FillSlotsWithNone(void *dest, u32 count);
extern void  FUN_08055d90(u32 *head, u32 id);

/* 0x080539F4 */
void FUN_080539f4(Node *node)
{
    SlotDesc *desc;
    u16      *slotB;
    u16      *slotA;
    int       i;

    desc = node->desc;

    slotB = node->slotsB;
    for (i = 0; i < desc->countB; i++, slotB++) {
        u16   id = *slotB;
        Node *entry = FUN_08054744(id);

        if (entry->dirty && entry->level == LEVEL_BASE)
            FUN_080536bc(entry);
        FUN_08055d90((u32 *)&gRam02035780, id);
    }

    slotA = node->slotsA;
    for (i = 0; i < desc->countA; i++, slotA++) {
        u16   id = *slotA;
        Node *entry = FindNode(id);

        if (entry != 0) {
            if (entry->dirty) {
                if (entry->level == LEVEL_BASE) {
                    u16 *inner = entry->slotsA;
                    int  j;

                    for (j = 0; j < entry->desc->count; j++, inner++)
                        FUN_08052750(*inner, 0);
                    FillSlotsWithNone(entry->slotsA, entry->desc->count);
                    entry->dirty = 0;
                }
            }
            FUN_08055d90((u32 *)&gNodeListHead, id);
        }
    }

    FillSlotsWithNone(node->slotsB, desc->countB);
    FillSlotsWithNone(node->slotsA, desc->countA);
}
