/* Alani sifirlayip kimlik dizisini yeniden kurma — 0x08052E78-0x0805302F
 * (440 bayt; son 22 bayt literal havuzu)
 *
 * Alanin +0x28'indeki tanim blogu iki sey tasiyor: +0x05 girdi sayisi ve
 * +0x0C u16 kimlik dizisi (sablon). Fonksiyon once alanin seviye nibble'ini
 * sifirliyor, sonra sablondaki her kimlik icin:
 *
 *   1) ROM bankasindaki (gAreaBank +0x24) 36 baytlik alan girisini aliyor;
 *      girisin +0x06 yuva sayisi kadar +0x10 yuva dizisini geziyor. Her
 *      yuva kimligi icin:
 *        - FUN_08055954 ile isaret nesnesi aranip bulunursa +0x18 bayrak
 *          kelimesi 0xE3FF0000 ile maskeleniyor,
 *        - FUN_08052750 cagriliyor,
 *        - ayni kimlikle bankanin +0x1C alanindaki 64 baytlik alt kayit
 *          aliniyor; onun +0x0D yuva sayisi sifir degilse +0x10 dizisi de
 *          ayni iki adimla geziliyor.
 *   2) Kimlik sirali dugum listesinde aranıyor (FindNode). Dugum varsa:
 *        - kirliyse (bit 0) dugumun kendi yuva dizisi FUN_08052750 ile
 *          gezilip FillSlotsWithNone ile bos kimlige cekiliyor ve kirli
 *          biti temizleniyor (nodelist_c2.c / d1.c ile ayni govde),
 *        - seviyesi 1'den BUYUKSE 1'e cekiliyor,
 *        - gNodeListHead o kimlikle tazeleniyor.
 *
 * Son olarak alanin +0x30 cikti dizisi varsa sablon yeniden gezilip her
 * kimlik oraya yaziliyor; 0x7FFF'ten buyuk kimlikler alanin +0x18'indeki
 * yeniden esleme tablosundan geciriliyor (kimlik - 0x8000 indeksiyle).
 * Yazilan kimlik FUN_08052C68'e veriliyor.
 *
 * YAPI IPUCLARI:
 *   - gAreaBank (0x08D49C00) +0x24 = 36 baytlik giris dizisi
 *     (src/core/nodelist_c3.c, src/core/nodelist_d5.c ile ayni),
 *     +0x1C = 64 baytlik alt kayit dizisi (BU DOSYADA ILK KEZ gorunuyor).
 *     Taban ROM'da DUZ yuklenip ofset yukleme komutunda birakiliyor
 *     (`ldr r2,=0x08D49C00` + `ldr r1,[r2,#36]`), yani YAPI UYESI erisimi;
 *     sabit cast yazmak ofseti havuz sabitine katlar (kural 1).
 *   - Alanin tanim blogu src/core/nodelist_d2.c'deki SlotDesc ile ayni:
 *     +0x05 countA (alanin kimlik sayisi), +0x06 countB (dugumun yuva
 *     sayisi). +0x0C sablon dizisi bu dosyada eklendi.
 *
 * UYGULANAN KURALLAR:
 *   - Kural 1: gAreaBank / gNodeListHead extern sembol.
 *   - Kural 2: `entry = &gAreaBank.entries[id]` biciminde ARA ISARETCI;
 *     ROM tabani r7'de tutup `ldr r0,[r7,#16]` / `ldrb r1,[r7,#6]` ile
 *     uyelere gidiyor.
 *   - Kural 9/31: sayaclar `int`; ROM `blt`/`bge` (isaretli) uretiyor.
 *     Buna karsilik 0x7FFF karsilastirmasi ROM'da `bls` (ISARETSIZ), bu
 *     yuzden oradaki kimlik yereli `u32`.
 *   - Kural 24/26: alanin ve dugumun +0x0B baytlari bitfield.
 *     `area->level = 0` -> `movs #15 / ands / strb`;
 *     `node->dirty = 0` -> `movs #2 / negs`;
 *     `node->level > 1` -> ISARETLI 4 bitlik alan, `lsls #24 / asrs #28`.
 *   - Kural 11: kimlik FindNode cagrisini asiyor -> ayri `u32` yerel
 *     (ROM r7'de tutuyor; `ldrh r7,[r2]` + `adds r0,r7,#0` sirasi
 *     nodelist_d3.c'deki olcumun `u32` yonu).
 *   - Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *   - Ic yuva dizileri ISARETCI YURUTMEYLE degil INDEKSLE geziliyor
 *     (ROM `lsls r4,r6,#1` ile j*2'yi bir kez kurup uc erisimde
 *     paylasiyor); dugumun kendi dizisi ise yurutuluyor (`adds r5,#2`).
 *
 * OLCULEN TEK AYRINTI — kural 43'un TERS YONU (16 bayt fark -> 0):
 *   Ilk dis dongunun artirimlari `i++, ids++` yazildiginda 16 bayt fark
 *   kaliyordu; komutlar dogru, yalniz SIRALARI tersti. Bu dongude
 *   artirimlar govdenin BASINA kaldirilip yigina tasiyor (sp+4 = i+1,
 *   sp+8 = ids+2) ve dip tarafta geri okunuyor. agbcc bu iki tasiyiciyi
 *   KAYNAK SIRASINDA yayiyor, yani ROM'un
 *       mov r0,r8 / adds r0,#2 / str r0,[sp,#8] / adds r5,#1 / str r5,[sp,#4]
 *   sirasi ancak `ids++, i++` ile cikiyor. Ikinci dis dongude ise ROM
 *   sayaci once artiriyor, orada `i++, out++, ids++` dogru sira.
 *   Yani kural 43 "sayac once" diye ezberlenemez: her dongude ROM'un
 *   kendi artirim sirasi okunup kaynaga o sirayla yazilmali.
 *   nodelist_d1.c/d2.c'de sira sayac-once idi; burada tersi.
 *
 * ESLESME: 440/440 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_d6.c
 */

#include "gba_types.h"

#define LEVEL_BASE   1
#define MARKER_KEEP  0xE3FF0000
#define REMAP_BASE   0x8000

/* src/core/nodelist_d2.c'deki SlotDesc ile ayni; +0x0C eklendi. */
typedef struct SlotDesc {
    u8   pad00[5];
    u8   countA;                /* +0x05 alanin kimlik sayisi */
    u8   countB;                /* +0x06 dugumun yuva sayisi  */
    u8   pad07[5];
    u16 *ids;                   /* +0x0C sablon kimlik dizisi */
} SlotDesc;

/* src/core/nodelist_d1.c / d5.c ile ayni yerlesim. */
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

/* 0x02035A80 listesindeki nesne; burada yalniz +0x18 bayrak kelimesi
 * kullaniliyor. */
typedef struct Marker {
    u8  pad00[0x18];
    u32 flags;                  /* +0x18 */
} Marker;

/* 36 baytlik alan girisi; src/core/nodelist_c3.c ile ayni, +0x06 ve +0x10
 * bu dosyada adlandirildi. */
typedef struct AreaEntry {
    u8   pad00[6];
    u8   count;                 /* +0x06 */
    u8   pad07[9];
    u16 *slots;                 /* +0x10 */
    u8   pad14[4];
    u16  areaFlag;              /* +0x18 */
    u16  checkId;               /* +0x1A */
    s32  mask;                  /* +0x1C */
    u8   unk20[3];
    u8   flags;                 /* +0x23 */
} AreaEntry;

/* 64 baytlik alt kayit. */
typedef struct AreaSub {
    u8   pad00[13];
    u8   count;                 /* +0x0D */
    u8   pad0E[2];
    u16 *slots;                 /* +0x10 */
    u8   pad14[0x2C];
} AreaSub;

typedef struct AreaBank {
    u8         unk00[0x1C];
    AreaSub   *subs;            /* +0x1C */
    u8         pad20[4];
    AreaEntry *entries;         /* +0x24 */
} AreaBank;

typedef struct Area {
    u8        pad00[11];
    u8        pad0B : 4;        /* +0x0B bit 0..3 */
    s8        level : 4;        /* +0x0B bit 4..7 */
    u8        pad0C[12];
    u16       remap[8];         /* +0x18 */
    SlotDesc *desc;             /* +0x28 */
    u8        pad2C[4];
    u16      *out;              /* +0x30 */
} Area;

extern AreaBank gAreaBank;
extern Node    *gNodeListHead;

extern Marker *FUN_08055954(u32 id);
extern Node   *FindNode(u32 id);
extern void    FUN_08052750(u32 a, u32 b);
extern void    FillSlotsWithNone(void *dest, u32 count);
extern void    FUN_08055d90(u32 *head, u32 id);
extern void    LinkAreaEntryIfEligible(s32 index);

/* 0x08052E78 */
void ResetAreaIds(Area *area)
{
    SlotDesc  *desc;
    int        i;
    int        j;
    int        k;
    int        m;
    u16       *ids;
    u16       *out;
    u16       *slot;
    AreaEntry *entry;
    AreaSub   *sub;
    Marker    *marker;
    Node      *node;
    u32        id;
    u32        value;

    desc = area->desc;
    area->level = 0;
    if (desc->countA == 0)
        return;

    ids = desc->ids;
    for (i = 0; i < desc->countA; ids++, i++) {
        entry = &gAreaBank.entries[*ids];

        for (j = 0; j < entry->count; j++) {
            marker = FUN_08055954(entry->slots[j]);
            if (marker != 0)
                marker->flags &= MARKER_KEEP;
            FUN_08052750(entry->slots[j], 0);

            sub = &gAreaBank.subs[entry->slots[j]];
            if (sub->count == 0)
                continue;

            for (k = 0; k < sub->count; k++) {
                marker = FUN_08055954(sub->slots[k]);
                if (marker != 0)
                    marker->flags &= MARKER_KEEP;
                FUN_08052750(sub->slots[k], 0);
            }
        }

        id = *ids;
        node = FindNode(id);
        if (node == 0)
            continue;

        if (node->dirty) {
            slot = node->slots;
            for (m = 0; m < node->desc->countB; m++, slot++)
                FUN_08052750(*slot, 0);
            FillSlotsWithNone(node->slots, node->desc->countB);
            node->dirty = 0;
        }

        if (node->level > LEVEL_BASE)
            node->level = LEVEL_BASE;

        FUN_08055d90((u32 *)&gNodeListHead, id);
    }

    out = area->out;
    if (out == 0)
        return;

    ids = desc->ids;
    for (i = 0; i < desc->countA; i++, out++, ids++) {
        value = *ids;
        if (value > 0x7FFF)
            value = area->remap[value - REMAP_BASE];
        *out = value;
        LinkAreaEntryIfEligible(*out);
    }
}
