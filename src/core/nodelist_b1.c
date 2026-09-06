/* Dugumu ve bagli alt nesnelerini kapatma. 0x08052828, 352 bayt. ESLESTI.
 *
 * Kardes FUN_08052750 (src/core/nodelist_a4.c, 0x08052750) ile ayni aileden:
 * ayni `index >= gAreaBank.count` kapisi, ayni gList02035A80 liste basi, ayni
 * FindNodeAfter (0x08055A94) aramasi, ayni +0x18 bayrak kelimesi ve ayni
 * 0x020110C0 kuyruk cagrisi. Farklari: burada bir ON kapi daha var
 * (index == 0x7FEF), fonksiyon KENDINI cagiriyor (sekil listesindeki her alt
 * kimlik icin) ve +0x30'daki "sahip" dugumu de temizliyor.
 *
 * ROM'DAN OLCULEN AYRINTILAR
 *   - Ilk kapi `ldr r0,=0x7fef / cmp r2,r0 / bne` -- esitlik sinamasi, havuz
 *     sabiti. Kural 44 (karsilastirma sabitini yerele al) BURADA GEREKMIYOR:
 *     kanoniklestirme yalnizca `<`/`<=` sinamalarinda oluyor, `==` degismiyor.
 *   - +0x18 alani ISARETLI (s32): ikinci kapi `cmp r1,#0 / bge`, yani
 *     `bits < 0`. Maskeli yazim (`bits & 0x80000000`) bu dali uretmez.
 *   - +0x30 SAHIP DUGUMU ve ayni Node yerlesimini tasiyor: r4 uzerinden
 *     hem `[r4,#40]` (+0x28 alt nesne) hem `[r4,#24]` (+0x18 bayrak) okunuyor.
 *     Yani `Node *owner` -- ayri bir tip degil.
 *   - `owner = node->owner;` ATAMASI, `bits < 0` KAPISINDAN ONCE olmali:
 *     ROM `ldr r4,[r5,#48]`i `ldr r1,[r5,#24]`den once yayiyor (kural 11/16).
 *   - Dongu bicimi: `movs r4,#0 / b .Lcond` ile DONDURULMUS for. On-baslikta
 *     sekil yuklemesi YOK; sekil kosul blogunda okunup govdeye CSE ediliyor
 *     (govde `ldr r0,[r0,#16]` ile dogrudan kosuldaki r0'i kullaniyor).
 *     Kardes a4'te ROM'un on-baslikta ucuncu bir yukleme yaptigi icin ORADA
 *     dal-ici ayri yerel (`Node *n = node;`) gerekiyordu; BURADA GEREKMIYOR --
 *     kural 49'un "dongu bicimini kardesten kopyalama" uyarisinin ornegi.
 *     Dogrulamasi prologda: burada `push {r4,r5,r6,lr}` (r7 yok), a4'te
 *     `push {r4-r7,lr}`.
 *   - 0x08041EE0 BIR ARGUMAN ALIYOR. Ikinci cagri oncesi acik `adds r0,r2,#0`
 *     var; birinci cagri oncesi yok cunku alt nesne zaten r0'a dagitilmis.
 *     src/core/nodelist_a8.c orayi `void FUN_08041ee0(void)` diye bildirmis ve
 *     eslesmis (orada r0 tesadufen doluydu) -- BURADA argumansiz bildirim
 *     8 bayt fark veriyor (olculdu).
 *   - Maskeler int genisliginde: `movs #17 / negs` = ~0x10, `movs #129 / negs`
 *     = ~0x80. Alanlar u32 oldugu icin kural 47 bir sey gerektirmiyor.
 *   - Havuzdaki iki negatif sabit: 0xFFFFFEFF = ~0x100 ve 0xFFFFFEEF = ~0x110.
 *     Ikincisi ~0x111 DEGIL -- tek bayt farkla eslesmeyi bozan yer burasiydi.
 *   - Iki dal `node->bits |= <sabit>` kuyrugunu PAYLASIYOR (capraz atlama,
 *     0x08052964). Kural 29'un tersi: ROM birlesmeyi zaten yapmis, kaynakta
 *     duz if/else yazmak dogru.
 *
 * DENEYIP ELEDIGIM YAZIMLAR (olculdu)
 *   - `sub->anim` uc sifir saklamasi KAYNAK SIRASININ TERSINE yayiliyor.
 *     0x00,0x04,0x08 sirasiyla yazinca ROM'un 0x08,0x04,0x00 sirasi cikmiyor:
 *     2 bayt fark. Kaynaga tersten yazildi. (Kural 8'in ayni yondeki etkisi.)
 *   - `else if (arm != 0) {...} else {...}` diye ters yazim: fonksiyon 348
 *     bayta iniyor ve 80 bayt farkli. agbcc kosulu cevirmiyor; ROM'daki
 *     `cmp r6,#0 / bne` ancak kaynakta ONCE `arm == 0` dali varken cikiyor.
 *   - `FUN_08041ee0()` argumansiz: 8 bayt fark (yukarida).
 *   - +0x0B alani `u8` vs `s8`: FARK ETMIYOR, ikisi de 0 fark veriyor.
 *     Alan yalniz `& 2` ile okunuyor, hicbir yerde yazilmiyor; kural 47'nin
 *     kaldiraci burada yok. a4'teki `s8` secimiyle tutarli kalsin diye `s8`
 *     birakildi -- bu dosyada KANIT DEGIL, yalnizca uyum.
 *
 * ACIK KALAN -- SEMBOL BILDIRIMI GEREKIYOR
 *   0x020110C0 data/ram_map.csv'de YOK (a4 ayni bosluk icin ayni notu
 *   dusmustu). Yalniz ADRESI arguman olarak geciyor, uye erisimi yok, bu
 *   yuzden ham cast havuz sabitini bozmuyor. ram_map'e dokunmadim.
 *   Alt nesnedeki 0x80/0x01/0x100/0x40/0x10/0x110/0x4000 bitlerinin anlami
 *   bilinmiyor; SUB_A..SUB_G diye notrsel adlandirildi, anlam UYDURULMADI.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_b1.c  -> BYTE-MATCHING
 */
#include "gba_types.h"

#define INDEX_NONE   0x7FEF      /* ilk kapi: gecersiz kimlik */
#define KIND_BUSY    0x02        /* +0x0B bit 1 */
#define BITS_SIGN    0x80000000  /* +0x18 isaret biti: `cmp/bge` ile sinaniyor */
#define BITS_HOLD    0x40000000
#define BITS_LIST    0x100
#define BITS_SUB     0x400
#define SUB_A        0x80
#define SUB_B        0x01
#define SUB_C        0x100
#define SUB_D        0x40
#define SUB_E        0x10
#define SUB_F        0x110
#define SUB_G        0x4000

typedef struct Anim {
    u32 unk00;
    u32 unk04;
    u32 unk08;
} Anim;

typedef struct Sub {
    u8    pad00[12];
    u32   flags;                /* 0x0C */
    u8    pad10[8];
    Anim *anim;                 /* 0x18 */
} Sub;

typedef struct Shape {
    u8   pad00[13];
    u8   count;                 /* 0x0D */
    u8   pad0E[2];
    u16 *list;                  /* 0x10 */
} Shape;

typedef struct Node {
    struct Node *next;          /* 0x00 */
    u8   pad04[7];
    s8   kind;                  /* 0x0B */
    u8   pad0C[12];
    s32  bits;                  /* 0x18 */
    u8   pad1C[4];
    u32  unk20;                 /* 0x20 */
    u8   pad24[4];
    struct Sub  *sub;           /* 0x28 */
    struct Shape *shape;        /* 0x2C */
    struct Node *owner;         /* 0x30 */
} Node;

typedef struct AreaBank {
    u8  pad00[4];
    s32 count;                  /* 0x04 */
} AreaBank;

extern AreaBank gAreaBank;              /* 0x08D49C00 (ROM tablosu) */
extern Node    *gList02035A80;          /* 0x02035A80 */

#define gRam020110C0 ((u32 *)0x020110C0)

extern Node *FindNodeAfter(Node *node, s32 id);      /* 0x08055A94 */
extern s32   FUN_08055888(Node *node, s32 mode);     /* 0x08055888 */
extern void  FUN_08041ee0(Sub *sub);                 /* 0x08041EE0 */
extern void  FUN_0800c804(u32 *dest, u32 value);     /* 0x0800C804 */

/* 0x08052828 */
void FUN_08052828(s32 index, u32 arm)
{
    Node *node;
    Node *owner;
    Sub  *osub;
    Sub  *sub;
    s32   i;

    if (index == INDEX_NONE) return;
    if (index >= gAreaBank.count) return;

    node = FindNodeAfter((Node *)&gList02035A80, index);
    if (node == 0) return;
    if ((node->kind & KIND_BUSY) != 0) return;

    owner = node->owner;
    if (node->bits < 0) return;
    if ((node->bits & BITS_HOLD) != 0 && arm == 0) return;

    if (owner != 0 && owner->sub != 0 && (owner->bits & BITS_SUB) != 0
        && FUN_08055888(owner, 0) == 0) {
        osub = owner->sub;
        osub->flags &= ~SUB_E;
        FUN_08041ee0(osub);
    }

    if ((node->bits & BITS_LIST) != 0) {
        for (i = 0; i < node->shape->count; i++)
            FUN_08052828(node->shape->list[i], 1);
    }

    sub = node->sub;
    if (sub != 0) {
        if ((node->bits & BITS_SUB) != 0) {
            if (sub->anim != 0) {
                sub->anim->unk08 = 0;
                sub->anim->unk04 = 0;
                sub->anim->unk00 = 0;
            }
            if ((sub->flags & SUB_A) != 0)
                sub->flags &= ~SUB_A;
            if ((sub->flags & SUB_B) != 0)
                sub->flags = (sub->flags & ~SUB_C) | SUB_D;
            sub->flags &= ~SUB_E;
            FUN_08041ee0(sub);
        } else if (arm == 0) {
            sub->flags = (sub->flags & ~SUB_F) | SUB_G;
            node->bits |= BITS_HOLD;
        } else {
            sub->flags |= BITS_SUB;
            node->bits |= BITS_SIGN;
        }
    }

    if (node->unk20 != 0) {
        FUN_0800c804(gRam020110C0, node->unk20);
        node->unk20 = 0;
    }
    node->owner = 0;
}
