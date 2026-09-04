/* Deger azaltma ve esik sinamasi — 0x08023974-0x08023A0B
 *
 * Uc kapidan gecerse degeri azaltip tabana kirpiyor, komsu bayrakliysa
 * ikinci bir esikle daha kirpiyor, sonra dort girisli ROM tablosunda
 * kaydirmali karsilastirma yapip bayragi temizliyor.
 *
 * gRam02000224 AYNI ADRESTEN iki farkli genislikte okunuyor: kapida
 * `ldrb` (bayt), uygulamada `ldr` (soz). Ikisi de kaynakta ayri ifade
 * olarak yazilmali.
 *
 * Sabit kaliplari: 0x1000000 = `0x80 << 17`, 0x20000 = `0x80 << 10`
 * (kaydirmayla); 0x0063FFFF ve 0x0001FFFF havuzdan.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 donus degeri tasiyor, imza u32.
 *
 * HENUZ ESLESMIYOR: 145/156. Bu yeniden kurulum YAPISAL OLARAK YANLIS
 * olmali -- boyut bile tutmuyor ve farklarin cogunlugu basta. Asagidaki
 * gozlemler ROM'dan DOGRUDAN okundu ve gecerli, ama bunlari birlestiren
 * kontrol akisi yanlis cikarilmis:
 *   - gRam02000224 ayni adresten IKI genislikte okunuyor (kapida `ldrb`,
 *     uygulamada `ldr`)
 *   - 0x1000000 = `0x80 << 17`, 0x20000 = `0x80 << 10` (kaydirmayla)
 *   - 0x0063FFFF ve 0x0001FFFF havuzdan
 *   - son dongu 0x08342AC8'deki dort girisli ROM tablosunda donuyor ve
 *     `asrs r0, r1` ile REGISTER MIKTARLI kaydirma yapiyor
 * KONTROL AKISI ARACLA CIKARILDI (tools/dump_cfg.py) ve kaynak ona gore
 * YENIDEN YAZILDI. Grafik 21 blok gosterdi; onemli olan BIRLESME
 * NOKTALARIYDI: B7'ye uc yerden, B15'e DORT yerden geliniyor. Yani ic ice
 * kosul degil, KORUMALI ERKEN CIKIS zinciri. Kaynak buna gore etiketli
 * yazildi (goto no / apply / peers / scan).
 *
 * Yine de eslesmedi: 145/156 (ROM 152). Yeni teshis:
 *     bizim `push {r4,r5,r6,r7,lr}`   ROM `push {r4,r5,r6,lr}`
 * Bir fazla callee-saved register, yani bir fazla canli deger.
 * `value` yerelini `span` ile paylastirmak DENENDI, hicbir sey degismedi.
 *
 * tools/dump_alloc.py: 22 pseudo-register / 5 spill. Karsilastirma icin
 * eslesen EngageActor 12 pseudo / 3 spill. Basinc iki kati; bu, birkac
 * baytlik bir duzeltmeyle kapanacak bir sey degil, kaynak yapisinin
 * kendisi ROM'unkinden daha fazla ara deger uretiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/step_decay.c
 */

#include "gba_types.h"

#define DELTA_MAX   0x0063FFFF
#define PEER_BIT    (0x80 << 17)
#define SPAN_LIMIT  0x0001FFFF
#define SPAN_RESET  (0x80 << 10)
#define TABLE       ((u32 *)0x08342AC8)
#define TABLE_LAST  3

typedef struct Obj {
    u8  tag;                    /* +0x00 */
    u8  pad01[3];
    s32 limit;                  /* +0x04 */
    s32 value;                  /* +0x08 */
    u32 stamp;                  /* +0x0C */
} Obj;

typedef struct Peer {
    u8  pad00[24];
    u32 flags;                  /* +0x18 */
} Peer;

typedef struct Node {
    u8    pad00[44];
    Peer *peer;                 /* +0x2C */
} Node;

extern u32 gRam02000224;

/* 0x08023974 */
/* 0x08023974 */
u32 StepDecay(Obj *obj, s32 delta, u32 unused, Node *node)
{
    Peer *peer;
    s32 limit;
    s32 span;
    u32 i;

    peer = 0;
    if (node != 0)
        peer = node->peer;

    if (delta == 0)
        goto no;

    span = obj->value;
    limit = obj->limit;
    if (span >= limit)
        goto apply;
    if (delta > DELTA_MAX)
        goto apply;
    if (*(u8 *)&gRam02000224 != obj->stamp)
        goto apply;

no:
    return 0;

apply:
    obj->stamp = gRam02000224;
    span = obj->value;
    if (span == 0)
        goto peers;
    obj->value = span - delta;
    if (obj->value > 0)
        goto peers;
    obj->value = 1;

peers:
    if (peer == 0)
        goto scan;
    if ((peer->flags & PEER_BIT) == 0)
        goto scan;
    if (span <= SPAN_LIMIT)
        goto scan;
    if (obj->value > SPAN_LIMIT)
        goto scan;
    obj->value = SPAN_RESET;

scan:
    i = 0;
    span = obj->value;
    do {
        if (span <= (limit >> TABLE[i]))
            obj->tag = i;
        i++;
    } while ((s32)i <= TABLE_LAST);

    return 1;
}
