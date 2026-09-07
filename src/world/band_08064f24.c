/* Izleme turunu kapatan puanlama adimi — 0x08064F24-0x0806512F, 524 bayt
 *
 * src/world/actor_tracking.c ile AYNI aktor yapisi uzerinde calisiyor:
 * ResetActorTracking (0x08065378) turu baslatiyor, AccumulateActorMotion
 * (0x080653D4) her karede birikimleri artiriyor, bu fonksiyon da turu
 * kapatiyor. Sirasiyla:
 *
 *   1. Sahibin oyuncu yuvasindaki +0x25 isaretini okuyup TEMIZLIYOR.
 *      Isaret kuruluysa tur puanlaniyor, degilse hicbir sey yapilmiyor.
 *   2. Aktorun yon bayti (+0x138) 0..3 ise kat edilen mesafe o eksende
 *      isaretli olarak olculuyor ((fark * 10) >> 6), yon bayti gecersizse
 *      (tur kapaliysa 0xFF) x ekseninde mutlak fark >> 11 kullaniliyor.
 *   3. Zemin degeri (+0x60) baslangic z'sinin altindaysa en buyuk dusus
 *      (+0x128) aradaki farkla azaltiliyor.
 *   4. Mesafe, dusus ve uc hiz birikimi (>>10) agirlikli toplam
 *      yardimcisina (FUN_080627ec) veriliyor.
 *   5. Mesafe pozitifse sahibin sayaci (+0x30 -> +0x08) mesafeye gore
 *      1/4, 2/4 ya da 3/4 adim kadar dusuruluyor ve 0x10000 tabaninda
 *      tutuluyor; sahibin dugumune (+0x2C) 0x1000 biti ekleniyor, dugumun
 *      alt kaydi 3 turundeyse ayni sayac 0x10000 TAVANINA cekiliyor.
 *   6. PlaceProbeEntries dortten fazla giris uretirse sayac yine tavana
 *      cekiliyor.
 *   7. Son olarak izleme sifirlaniyor (ResetActorTracking'in govdesinin
 *      birebir ayni satirlari) ve olculen mesafe yuvanin +0x4C alanina
 *      yaziliyor.
 *
 * OLCULEN YAZIM KURALLARI (hepsi bu fonksiyonda denendi, hepsi gerekli):
 *
 * A. switch govdeleri ROM'daki blok sirasina gore yazildi: 0, 2, 1, 3
 *    (kural 61 eki). Kaynak sirasi 0,1,2,3 olsa bloklar ters cikiyor.
 *
 * B. 2 ve 1 numarali durumlarda ROM ONCE baslangic konumunu yukluyor.
 *    `pos.y - startPos.y` yazimi once `pos.y`'yi yukluyor (249/251).
 *    `-startPos.y + pos.y` yazimi ROM'un sirasini veriyor. Elenen: unary
 *    `-(startPos.y - pos.y)` (agbcc agac duzeyinde katliyor, fark yok),
 *    `10 * (...)` (fark yok), `(startPos.y - pos.y) * -10` (532 bayt),
 *    `/ (1 << 6)` (548 bayt), her durumda ara `diff` yereli (221/251).
 *
 * C. FUN_080627ec'in 6. argumani DEGISKEN olmali. Dogrudan `0` yazarsan
 *    agbcc sabiti yigin yazimindan SONRA uretiyor, boylece besinci
 *    arguman yazmacta beklemiyor, blok bir yazmac az istiyor ve TUM
 *    fonksiyonun dagitimi kayiyor (actor r4'e dusuyor, ROM'da r6):
 *    115/251. Degisken olunca ROM gibi r0-r5'in alti da dolu oluyor.
 *    Ayni gerekce ile ucuncu/dorduncu/besinci arguman da yerel:
 *    hepsi satir ici yazilinca 2. arguman once hesaplaniyor (89/251).
 *
 * D. Sayac blogunda ARA ISARETCI KULLANILMAMALI. `counter = owner->counter`
 *    yerelini her kola yazmak 194/251 veriyor; `owner->counter->` diye
 *    dogrudan yazmak 225/251 ve ROM'un yazmac dagitimini
 *    (counter r1, tavan r2, dusus r3) getiriyor. Elenen: tek ortak
 *    `counter` yereli (192), her kolda tam guncelleme (207).
 *
 * E. 0x02035B10 durum sozcugu ROM'da OKUNUP ATILIYOR: `ldr r0,=..` /
 *    `ldr r0,[r0]` sonrasi r0 hemen eziliyor. Olu yuklemeyi ayakta
 *    tutmanin tek yolu volatile gorunum (ram_symbols.h'nin "her
 *    translation unit kendi gorunumunu bildirir" kuralina uygun).
 *    Ayrica bu okuma r0'i tuttugu icin FUN_08067370'in argumani r1'de
 *    hesaplanip r0'a kopyalaniyor; cagriya AYNI degerin iki kez
 *    verilmesi (r0 ve r1) ROM'un `adds r0,r1,#0` kopyasini veren tek
 *    yazim. Elenen: tek argumanli cagri (250/251, bir komut eksik),
 *    `FUN_08067370(gRam02035B10, dist>>16)` (249), ucuncu arguman `0`
 *    (250), virgul operatoru, ara `scaled` yereli.
 *
 * ESLESME: 524/524 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/band_08064f24.c
 */

#include "gba_types.h"

#define TRACK_MARKER    0xFF

#define DIST_NUM        10
#define DIST_SHIFT      6
#define PLAIN_SHIFT     11
#define ACCUM_SHIFT     10

#define DIST_STEP       (320 << 16)
#define COUNTER_LIMIT   (128 << 9)
#define NODE_FLAG       (128 << 5)
#define SUB_KIND        3
#define PROBE_LIMIT     3

typedef struct Vec3 {
    s32 x;                              /* +0x00 */
    s32 y;                              /* +0x04 */
    s32 z;                              /* +0x08 */
} Vec3;

/* SelectSlotAB'nin dondurdugu oyuncu yuvasi; +0x25 isareti
 * src/world/actor_tracking.c ile ayni alan. */
typedef struct PlayerSlot {
    u8  pad00[0x25];
    u8  marked;                         /* +0x25 */
    u8  pad26[0x4C - 0x26];
    s32 distance;                       /* +0x4C */
} PlayerSlot;

/* src/world/spawn_slot_effect.c'deki SlotCounter; orada yalnizca +0x08
 * kullaniliyordu, burada +0x04 adimi da okunuyor. */
typedef struct SlotCounter {
    u8  pad00[4];
    s32 step;                           /* +0x04 */
    s32 value;                          /* +0x08 */
} SlotCounter;

typedef struct SubNode {
    u8  pad00[0x26];
    u16 kind;                           /* +0x26 */
} SubNode;

typedef struct Node {
    u8       pad00[0x1E];
    u16      flags;                     /* +0x1E */
    u8       pad20[0x2C - 0x20];
    SubNode *sub;                       /* +0x2C */
} Node;

/* Aktorun +0x64 sahibi: src/world/spawn_slot_effect.c'deki ActiveSlot
 * (orada da +0x30 sayaci ayni 0x10000 sinirina cekiliyor). */
typedef struct ActiveSlot {
    u8           pad00[0x2C];
    Node        *node;                  /* +0x2C */
    SlotCounter *counter;               /* +0x30 */
} ActiveSlot;

/* src/world/actor_tracking.c'deki TrackedActor; ek olarak +0x60 zemin
 * degeri ve sahibin alt yapilari kullaniliyor. */
typedef struct TrackedActor {
    Vec3        pos;                    /* +0x000 */
    u8          pad0C[0x60 - 0x0C];
    s32         ground;                 /* +0x060 */
    ActiveSlot *owner;                  /* +0x064 */
    u8          pad68[0x118 - 0x68];
    Vec3        startPos;               /* +0x118 */
    s32         spare;                  /* +0x124 */
    s32         maxDelta;               /* +0x128 */
    s32         accumB;                 /* +0x12C */
    s32         accumC;                 /* +0x130 */
    s32         accumA;                 /* +0x134 */
    u8          marker;                 /* +0x138 */
} TrackedActor;

extern u32   GetOwnerSlot(void *owner);
extern void *SelectSlotAB(u32 which);
extern void  AreaFlagsNoop(u32 marked);
extern void  FUN_08067370(s32 value, s32 target);
extern void  FUN_080627ec(s32 dist, s32 drop, s32 rateB, s32 rateC,
                          s32 rateA, s32 rateD);
extern s32   PlaceProbeEntries(void *actor, s32 mode);

/* Bkz. dosya basi, madde E: bu sozcuk okunup atiliyor, olu yuklemenin
 * kalmasi icin bu TU'da volatile gorunum bildiriliyor. */
/* Tur, src/core/reset_runtime_globals.c ile AYNI olmali (tutarlilik
 * denetimi ayni sembol icin celiskili extern turlerini durduruyor).
 * ROM buradaki olu okumayi yapiyor; volatile ifade duzeyinde
 * veriliyor -- src/world/link_service.c ile ayni kalip. */
extern u32 gRam02035B10;

/* 0x08064F24 */
void FUN_08064f24(TrackedActor *actor)
{
    PlayerSlot *slot;
    PlayerSlot *reset;
    ActiveSlot *owner;
    Node       *node;
    u8          marked;
    s32         dist;
    s32         diff;
    s32         drop;
    s32         rateA;
    s32         rateB;
    s32         rateC;
    s32         rateD;

    slot = (PlayerSlot *)SelectSlotAB(GetOwnerSlot(actor->owner));
    if (slot != 0) {
        marked = slot->marked;
        slot->marked = 0;
    } else {
        marked = 0;
    }

    if (marked == 0)
        return;

    switch (actor->marker) {
    case 0:
        dist = (actor->startPos.y - actor->pos.y) * DIST_NUM >> DIST_SHIFT;
        break;
    case 2:
        dist = (-actor->startPos.y + actor->pos.y) * DIST_NUM >> DIST_SHIFT;
        break;
    case 1:
        dist = (-actor->startPos.x + actor->pos.x) * DIST_NUM >> DIST_SHIFT;
        break;
    case 3:
        dist = (actor->startPos.x - actor->pos.x) * DIST_NUM >> DIST_SHIFT;
        break;
    default:
        diff = actor->startPos.x - actor->pos.x;
        if (diff < 0)
            diff = -diff;
        dist = diff >> PLAIN_SHIFT;
        break;
    }

    if (actor->ground < actor->startPos.z)
        actor->maxDelta -= actor->startPos.z - actor->ground;

    rateB = actor->accumB >> ACCUM_SHIFT;
    rateC = actor->accumC >> ACCUM_SHIFT;
    rateA = actor->accumA >> ACCUM_SHIFT;
    rateD = 0;
    FUN_080627ec(dist, actor->maxDelta, rateB, rateC, rateA, rateD);

    if (dist > 0) {
        owner = actor->owner;
        AreaFlagsNoop(marked);
        (void)*(volatile u32 *)&gRam02035B10;
        FUN_08067370(dist >> 16, dist >> 16);

        if (dist > DIST_STEP * 3 - 1)
            drop = owner->counter->step * 3 >> 2;
        else if (dist > DIST_STEP * 2 - 1)
            drop = owner->counter->step * 2 >> 2;
        else if (dist > DIST_STEP - 1)
            drop = owner->counter->step >> 2;
        else
            drop = 0;

        owner->counter->value -= drop;
        if (owner->counter->value <= COUNTER_LIMIT)
            owner->counter->value = COUNTER_LIMIT;

        node = owner->node;
        if (node != 0) {
            node->flags |= NODE_FLAG;
            if (node->sub != 0 && node->sub->kind == SUB_KIND
                && owner->counter->value > COUNTER_LIMIT)
                owner->counter->value = COUNTER_LIMIT;
        }
    }

    if (PlaceProbeEntries(actor, 0) > PROBE_LIMIT) {
        if (actor->owner->counter->value > COUNTER_LIMIT)
            actor->owner->counter->value = COUNTER_LIMIT;
    }

    reset = (PlayerSlot *)SelectSlotAB(GetOwnerSlot(actor->owner));
    if (reset != 0)
        reset->marked = 0;

    actor->startPos = actor->pos;

    actor->maxDelta = 0;
    actor->accumB   = 0;
    actor->accumC   = 0;
    actor->accumA   = 0;
    actor->spare    = 0;

    actor->marker   = TRACK_MARKER;

    if (slot != 0)
        slot->distance = dist;
}
