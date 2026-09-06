/* Dugumu "hazir" kipe alip serbest birakma. 0x08052750, 216 bayt. ESLESTI.
 *
 * RebuildAreaEntry (src/core/nodelist_b6.c, 0x080526B8) ile ayni ailedendir;
 * 152 bayt otede oturuyor. Ortak olan: `index >= gAreaBank.count` kapisi,
 * gList02035A80 (0x02035A80) liste basi ve dugumun +0x0B "kind" bayti.
 * Farki: bu fonksiyon dugumu ARIYOR (FindNodeAfter, 0x08055A94), bir dizi
 * kapidan geciriyor ve sonunda FUN_08055D90 ile bildirim yapiyor.
 *
 * ROM'DAN OLCULEN AYRINTILAR
 *   - gAreaBank ROM'da 0x08D49C00; +0x04 s32 sayac (nodelist_c3/d5/d6 ile ayni
 *     gorunum). `ldr r0,=0x08D49C00` + `ldr r0,[r0,#4]`, yani duz uye erisimi.
 *   - Liste basi gList02035A80; hem aramaya hem bildirime ADRESI geciyor
 *     (`ldr r0,=0x02035A80`), degeri degil. b6 ile ayni kalip.
 *     (0x02035A70'deki gNodeListHead DEGIL -- komsu ama ayri nesne.)
 *   - Donus tipi VOID: cikista `pop {r4-r7}; pop {r0}; bx r0` ve hicbir yolda
 *     r0'a deger yazilmiyor (kural 35).
 *   - 0x100 maskesi `movs #128 / lsls #1`, 0x400 maskesi `movs #128 / lsls #3`
 *     olarak kuruluyor; ikisi de havuz sabiti degil.
 *   - Else dalinda `str r3` iki kez sifir yaziyor: r3 = bits & 0x100 ve dala
 *     ancak r3 == 0 iken giriliyor. Kaynakta duz `= 0` yeterli; agbcc'nin
 *     cse'si kosuldan gelen "yazmac sifir" esitligini taniyip r3'u yeniden
 *     kullaniyor, fazladan `movs` uretmiyor.
 *
 * ILK YAZIM 208 BAYT VERDI (103 komutun 45'i farkli). Iki ayri kaldirac
 * bulundu; ikisi de 8 baytin kaynagi:
 *
 * 1) +0x0B ALANI ISARETLI OLMALI (s8), kural 47'nin TERSI yonu.
 *    `u8 kind` ile `kind &= ~1` tek komuta katlaniyor: `movs r0,#254`.
 *    ROM ise `movs r0,#2 / negs r0,r0` ile -2 kuruyor, yani maske INT
 *    genisliginde. s8 yapinca ROM'daki iki komut cikiyor (+2 bayt).
 *    Ters yonde bir bedel YOK: `kind & 1`, `kind & 0xF1` ve
 *    `(kind & 0xF) | 0x10` hala `ldrb` uretiyor, agbcc 0x100'den kucuk
 *    maskede sign-extend eklemiyor. (b6 ayni alani s8 gormustu ve orada
 *    isaretliligin farketmedigini not etmisti -- BURADA FARKEDIYOR.)
 *
 * 2) DONGU DALINA AYRI YEREL ISARETCI (kural 45 kaliba uyuyor).
 *    `Node *n = node;` olmadan agbcc dugumu r5'te tutuyor, index'i r6'da
 *    tutuyor, r7'yi hic kullanmiyor ve dongude `node->shape` yuklemesini
 *    kosul blogundan govdeye CSE ediyor -- iterasyon basina TEK `ldr [.,#44]`.
 *    ROM'da iki tane var (govdede liste icin, latch'te sayac icin) ve
 *    on-baslikta ucuncusu. Dal-yerelini ekleyince ROM'un tam sekli cikiyor:
 *      adds r5,r6,#0 / movs r4,#0 / ldr r0,[r6,#44] / b .Lcond
 *    ve index r7'ye tasiniyor, `push/pop {r4-r7}` oluyor (+6 bayt).
 *    Yani `adds r5,r6,#0` YAZMAC KOPYASI kaynaktan URETILEBILIR bir
 *    kopyaydi: bedeli dal ici ayri yerel.
 *
 * DONGU BICIMI: agbcc'nin klasik dondurulmus for'u -- on-basta shape
 * yuklenip kosula dallaniliyor (`b .Lcond`), govde 0x080527C4, kosul
 * 0x080527D6. Kardes b6'da hic dongu yok; bicim ondan KOPYALANAMAZDI.
 *
 * DENEYIP ELEDIGIM YAZIMLAR
 *   - `u8 kind` (bkz. 1): 2 bayt eksik, `movs #254`.
 *   - Dal-yereli olmadan duz `node->...` (bkz. 2): 6 bayt eksik, dongu
 *     govdesi bir `ldr` kisa, r7 bos, `push {r4,r5,r6,lr}`.
 *   - `Shape *shape = node->shape;` diye dongu ONCESINE yerel almayi
 *     denemedim: ROM her turda yeniden okudugu icin bastan elenmis yazim
 *     (cagri arasi bellek gecersizlenmesi ROM'da acikca goruluyor).
 *
 * ACIK KALAN -- SEMBOL BILDIRIMI GEREKIYOR
 *   0x020110C0 data/ram_map.csv'de YOK (en yakini 0x020110AC =
 *   gRam020110AC, ayri literal). Burada yalniz ADRESI arguman olarak
 *   geciyor, uye erisimi yok, bu yuzden ham cast havuz sabitini bozmuyor
 *   ve eslesmeyi engellemiyor. ram_map'e dokunmadim; sembol adi
 *   verilmesi gerekiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_a4.c  -> BYTE-MATCHING
 */
#include "gba_types.h"

#define BITS_BUSY    0x02        /* +0x18 bit 1: mesgul -> hicbir sey yapma */
#define KIND_ARMED   0x01        /* +0x0B bit 0 */
#define KIND_LOW     0x0F
#define KIND_READY   0x10
#define KIND_TEST    0xF1
#define KIND_MATCH   0x11
#define BITS_LIST    0x100       /* +0x18 bit 8: sekil listesi yolu */
#define SUB_DONE     0x400       /* alt nesnenin +0x0C bayragi */

/* +0x2C'deki sekil tanimi; yalniz kullanilan alanlar adlandirildi. */
typedef struct Shape {
    u8   pad00[13];
    u8   count;                 /* 0x0D */
    u8   pad0E[2];
    u16 *list;                  /* 0x10 */
} Shape;

/* +0x28'deki alt nesne. */
typedef struct Sub {
    u8  pad00[9];
    u8  unk09;                  /* 0x09 */
    u8  pad0A[2];
    u32 flags;                  /* 0x0C */
    u8  pad10[0x1C];
    u32 unk2C;                  /* 0x2C */
} Sub;

typedef struct Node {
    struct Node *next;          /* 0x00 */
    u8   pad04[7];
    s8   kind;                  /* 0x0B */
    u8   pad0C[12];
    u32  bits;                  /* 0x18 */
    u16  unk1C;                 /* 0x1C */
    u8   pad1E[2];
    u32  unk20;                 /* 0x20 */
    u8   pad24[4];
    Sub *sub;                   /* 0x28 */
    Shape *shape;               /* 0x2C */
} Node;

typedef struct AreaBank {
    u8  pad00[4];
    s32 count;                  /* 0x04 */
} AreaBank;

extern AreaBank gAreaBank;              /* 0x08D49C00 (ROM tablosu) */
extern Node    *gList02035A80;          /* 0x02035A80 */

/* 0x020110C0'in data/ram_map.csv'de sembolu YOK (en yakini 0x020110AC =
 * gRam020110AC). Yalniz ADRESI arguman olarak geciyor, uye erisimi yok,
 * bu yuzden ham cast havuz sabitini bozmuyor. Sembol adi verilmesi gerek;
 * ram_map'e dokunmuyorum, raporda bildiriliyor. */
#define gRam020110C0 ((u32 *)0x020110C0)

extern Node *FindNodeAfter(Node *node, s32 id);   /* 0x08055A94 */
extern u32   GetOwnerSlot(u32 sub);               /* 0x0803C400 */
extern void  ForwardZeroArg2(u32 id);             /* 0x08055BF8 */
extern void  FUN_0800c804(u32 *dest, u32 value);  /* 0x0800C804 */
extern void  FUN_08055d90(u32 *head, u32 id);     /* 0x08055D90 */

/* 0x08052750 */
void FUN_08052750(s32 index, u32 arm)
{
    Node *node;
    s32 i;

    if (index >= gAreaBank.count) return;

    node = FindNodeAfter((Node *)&gList02035A80, index);
    if (node == 0) return;
    if ((node->bits & BITS_BUSY) != 0) return;
    if (node->sub != 0 && GetOwnerSlot((u32)node->sub) != 0) return;

    if ((node->kind & KIND_ARMED) != 0 && arm != 0)
        node->kind = (node->kind & KIND_LOW) | KIND_READY;

    if ((node->kind & KIND_TEST) == KIND_MATCH) {
        if ((node->bits & BITS_LIST) != 0) {
            Node *n = node;
            for (i = 0; i < n->shape->count; i++)
                ForwardZeroArg2(n->shape->list[i]);
            n->kind &= ~KIND_ARMED;
        } else {
            Sub *sub = node->sub;
            if (sub != 0) {
                node->unk1C = sub->unk09;
                sub->unk2C = 0;
                sub->flags |= SUB_DONE;
                node->sub = 0;
            }
        }

        if (node->unk20 != 0) {
            FUN_0800c804(gRam020110C0, node->unk20);
            node->unk20 = 0;
        }
    }

    FUN_08055d90((u32 *)&gList02035A80, index);
}
