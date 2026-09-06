/* Varlik animasyon dugumu secimi ve alt kurulum dagitimi -- 0x08015598-0x080157B7
 *
 * InitActor (src/world/actor_init.c) ile kurulan Actor yapisini alip ROM'daki
 * tanim agacindan (0x08BD3448 koku) bir dugum secip varliga bagliyor, sonra
 * kurulumun geri kalanini alti ayri fonksiyona dagitiyor.
 *
 * ROM'dan okunan akis:
 *   1. gRam03000078 -> yerel "idx" (0 veya 1). a->slots[idx] 0xFDFD ise
 *      (InitActor'un yazdigi "atanmamis" degeri) oteki yuvaya geciliyor.
 *   2. Kok -> grup -> giris -> dugum: uc kademe isaretci tablosu. Ayni uc
 *      kademeli erisim src/world/entries_a8.c'de de var, oradaki `desc` ile
 *      buradaki `node` ayni seviyedeki nesne (ikisi de FUN_08013cfc'ye ve
 *      FUN_08014ee4'e +0x14 alaniyla gidiyor).
 *   3. Ust baglam kurulumu engelliyorsa dugum yerine sabit bir ROM dugumu.
 *   4. a->attr bayraklari ve FUN_08014ee4 secimi, sonra alti alt cagri.
 *
 * BYTE-MATCHING.
 *
 * OLCULEN NOKTALAR:
 *   - 0x12'deki `ldrsh`: a->pos 16.16 sabit nokta, tamsayi kismi ayri bir s16
 *     alan olarak okunuyor. `(s16)(raw >> 16)` yazmak `asrs` uretiyor, ROM
 *     `ldrsh` yaziyor -- birlesim (union) sart, kaydirma ile olmuyor.
 *   - Isaretci gecerlilik testleri `(u32)p - taban <= uzunluk` biciminde:
 *     agbcc bunu `movs #0xFE; lsls #24; adds; cmp; bls` olarak kuruyor.
 *     Aralik sirasi ROM'dan: `a` icin EWRAM,IWRAM; node->unk10 icin
 *     ROM,EWRAM,IWRAM. Sirayi degistirmek fark birakiyor.
 *   - DAL SIRASI: iki yerde kosulu TERS yazmak gerekti. ROM'un ic (dusen)
 *     dali `IsEntityEngaged(...) == 0` ve `a->unk98 == 0` tarafi; dogru
 *     tarafini once yazinca 542 -> 538 -> 544 gitti. `!= 0` yazimi ayni
 *     komutlari uretiyor ama bloklari ters diziyor.
 *   - `self` KOPYASI GEREKLI (ROM: `mov r9, r4` girişte, `mov r0, r9` besinci
 *     cagrida). Kopya tabanli bolme genelde eleniyor; BURADA ELENMIYOR, cunku
 *     kopya 5. cagriya kadar hicbir yerde kullanilmiyor ve fonksiyonun geri
 *     kalani zaten r4-r7'yi doldurmus durumda, yani ikinci allocno r8/r9'a
 *     tasiniyor. `self` cikarilinca 544 -> 538 (prolog/epilog 4 bayt + hizalama).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/actor_b3.c
 */

#include "gba_types.h"

/* Bellek bolgeleri: isaretcinin gercekten okunabilir bir alana bakip
 * bakmadigi boylece sinaniyor (ROM'daki uc ardisik aralik testi). */
#define IN_ROM(p)    ((u32)(p) - 0x08000000 <= 0x00FFFFFF)
#define IN_EWRAM(p)  ((u32)(p) - 0x02000000 <= 0x0003FFFF)
#define IN_IWRAM(p)  ((u32)(p) - 0x03000000 <= 0x00007FFF)
#define IS_RAM_PTR(p)    (IN_EWRAM(p) || IN_IWRAM(p))
#define IS_LOADED_PTR(p) (IN_ROM(p) || IN_EWRAM(p) || IN_IWRAM(p))

/* a->unk30->unk0C bayraklari */
#define CTX_BLOCKED   0x00000400
#define CTX_ENABLED   0x00000001
#define CTX_OVERRIDE  0x00008000

/* Varsayilan dugum; ust baglam kurulumu engelledigi zaman kullaniliyor. */
#define FALLBACK_NODE ((RomNode *)0x08CA45A8)

#define SLOT_UNSET    0xFDFD    /* InitActor'un yazdigi "atanmamis" degeri */
#define MODE_SPECIAL  15
#define MODE_BIT      4
#define ATTR_BIT      8
#define NODE_KIND_MAX 2

/* 16.16 sabit nokta; tamsayi kismi ayrica s16 olarak okunuyor. */
typedef union Fixed16 {
    s32 raw;                    /* +0x00 */
    struct {
        u16 frac;               /* +0x00 */
        s16 whole;              /* +0x02 */
    } part;
} Fixed16;

/* --- ROM tanim agaci: her kademede +0x00 sayac, +0x04 isaretci tablosu --- */

typedef struct RomNode {
    u8   pad00[0x0C];
    u8   kind;                  /* +0x0C  1 veya 2 olmali */
    void *unk10;                /* +0x10  yuklenmis bir alana bakmali */
    u32  unk14;                 /* +0x14 */
} RomNode;

typedef struct RomEntry {
    u8        count;            /* +0x00  cerceve sayisi */
    u8        pad01[3];
    RomNode **nodes;            /* +0x04 */
} RomEntry;

typedef struct RomGroup {
    u32        count;           /* +0x00 */
    RomEntry **entries;         /* +0x04 */
} RomGroup;

typedef struct RomRoot {
    u32        pad00;
    RomGroup **groups;          /* +0x04 */
} RomRoot;

extern RomRoot gRom08BD3448;

/* --- RAM --- */

extern u32 gRam03000078;        /* secilecek yuva indisi (0/1) */
extern u32 gRam02000224;        /* genel kip bayraklari */

/* --- Baglam ve gorunum yapilari --- */

typedef struct Context {
    u8  pad00[0x0C];
    u32 flags;                  /* +0x0C */
} Context;

typedef struct AttrOwner {
    u8  pad00[3];
    u8  unk03;                  /* +0x03 */
} AttrOwner;

typedef struct Attr {
    u8         pad00[0x20];
    u16        bits;            /* +0x20 */
    u8         pad22[0x16];
    AttrOwner *owner;           /* +0x38 */
} Attr;

typedef struct Actor {
    u32      unk00;             /* +0x00 */
    u16      unk04;             /* +0x04  giris indisi */
    u16      unk06;             /* +0x06 */
    u8       unk08;             /* +0x08 */
    u8       mode;              /* +0x09 */
    u8       pad0A[2];
    s32      unk0C;             /* +0x0C  cerceve sayisi << 16 */
    Fixed16  pos;               /* +0x10 */
    u32      unk14;             /* +0x14 */
    u8       pad18[4];
    u32      unk1C;             /* +0x1C */
    u16      slots[2];          /* +0x20  0xFDFD = atanmamis */
    RomGroup *group;            /* +0x24 */
    u32      unk28;             /* +0x28 */
    u8       pad2C[4];
    Context *ctx;               /* +0x30 */
    u32      unk34;             /* +0x34 */
    u32      unk38;             /* +0x38 */
    Attr    *attr;              /* +0x3C */
    u8       pad40[0x50];
    u32      unk90;             /* +0x90 */
    u8       pad94[4];
    u32      unk98;             /* +0x98 */
    u8       pad9C[0x14];
    u8       attrSlot;          /* +0xB0 */
    u8       padB1[3];
    RomNode *node;              /* +0xB4 */
} Actor;

extern u32   FUN_08013cfc(Attr *attr, RomNode *node, u32 idx);
extern void  FUN_08014ee4(Attr *attr, u32 value);
extern Attr *FUN_08028f98(u8 slot);
extern u32   IsEntityEngaged(Context *ctx);
extern void  FUN_08015a84(Actor *a, RomNode *node, u32 idx);
extern void  FUN_08015af8(Actor *a, RomNode *node, u32 idx);
extern void  FUN_080159a0(Actor *a, RomNode *node, u32 idx);
extern void  FUN_080197fc(Actor *a, RomNode *node, u32 idx);
extern void  FUN_08015834(Actor *a, RomNode *node, u32 idx);
extern void  FUN_080157b8(Actor *a, u32 idx);

/* 0x08015598 */
void FUN_08015598(Actor *a)
{
    RomGroup *group;
    RomEntry *entry;
    RomNode *node;
    Attr *attr;
    AttrOwner *owner;
    s32 limit;
    u32 idx;
    u32 kind;
    Actor *self;

    self = a;               /* bkz. baslik: 5. cagri ROM'da r9 uzerinden */
    idx = gRam03000078;

    if (a->ctx->flags & CTX_BLOCKED)
        return;
    if (a == 0)
        return;
    if (!IS_RAM_PTR(a))
        return;
    if (a->ctx == 0)
        return;
    if ((a->ctx->flags & CTX_ENABLED) == 0)
        return;

    if (a->slots[idx] == SLOT_UNSET)
        idx = (idx == 0);

    group = gRom08BD3448.groups[a->slots[idx]];
    a->group = group;
    if (a->unk04 >= group->count)
        return;

    entry = group->entries[a->unk04];
    limit = entry->count << 16;
    a->unk0C = limit;
    if (a->pos.raw >= limit)
        a->pos.raw = limit - 1;

    node = entry->nodes[a->pos.part.whole];
    if (node == 0)
        return;

    kind = node->kind;
    if (kind == 0)
        return;
    if (kind > NODE_KIND_MAX)
        return;

    if (!IS_LOADED_PTR(node->unk10))
        return;

    if ((a->ctx != 0 && (a->ctx->flags & CTX_OVERRIDE)) ||
        (a->mode == MODE_SPECIAL && (gRam02000224 & MODE_BIT) == 0))
        node = FALLBACK_NODE;

    a->node = node;
    if (FUN_08013cfc(a->attr, node, idx) == 0)
        return;

    if (a->attr != 0) {
        if (IsEntityEngaged(a->ctx) == 0) {
            a->attr->bits &= ~ATTR_BIT;
            attr = FUN_08028f98(a->attrSlot);
            if (attr != 0)
                attr->bits &= ~ATTR_BIT;
        } else {
            a->attr->bits |= ATTR_BIT;
            attr = FUN_08028f98(a->attrSlot);
            if (attr != 0)
                attr->bits |= ATTR_BIT;
        }
    }

    if (a->unk98 == 0) {
        if (a->unk90 == 0) {
            FUN_08014ee4(a->attr, node->unk14);
        } else {
            FUN_08014ee4(a->attr, a->unk90);
        }
    } else {
        FUN_08014ee4(a->attr, a->unk98);
        owner = a->attr->owner;
        if (owner != 0)
            owner->unk03 = 1;
    }

    FUN_08015a84(a, node, idx);
    FUN_08015af8(a, node, idx);
    FUN_080159a0(a, node, idx);
    FUN_080197fc(a, node, idx);
    FUN_08015834(self, node, idx);
    FUN_080157b8(a, idx);
}
