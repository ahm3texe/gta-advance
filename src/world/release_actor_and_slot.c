/* Band B — 0x08030B34 .. 0x08031D23 arasindan dokuz fonksiyon.
 *
 * ONEMLI OLCUM NOTU — `make c-match FILE=src/world/band_b.c` CIKTISI YANILTICI
 * ----------------------------------------------------------------------------
 * tools/agbcc_build.py bir dosyadaki TUM fonksiyonlari PES PESE, en kucuk ROM
 * adresinden baslayarak linkliyor (`base = min(address)`, `SUBALIGN(1)`).  Bu
 * dokuz fonksiyon ROM'da BITISIK DEGIL; aralarinda baska ceviri birimlerine ait
 * fonksiyonlar var.  Sonuc: ilk fonksiyon (SendTextMode1) disindaki her fonksiyon
 * kendi ROM adresinden SABIT bir delta kadar kaymis olarak linkleniyor ve
 * govdesindeki her `bl` o delta kadar yanlis kodlaniyor.  Govde birebir dogru
 * olsa bile `bl` iceren fonksiyon "farkli: 2/N byte" gorunuyor.
 *
 * Bu yuzden her fonksiyon AYRICA tek fonksiyonluk bir dosyada (base = kendi ROM
 * adresi, `bl` dogru) olculdu.  Tek fonksiyonluk olcumler — dogru olanlar:
 *     SendTextMode1  12  BYTE-MATCHING
 *     LoadHudPalettes 124  BYTE-MATCHING
 *     TriggerEvent39  12  BYTE-MATCHING
 *     GetRecordNodeById  14  BYTE-MATCHING
 *     ScaleMagnitude 132  BYTE-MATCHING
 *     ClearHudRowsAB  72  BYTE-MATCHING
 *     ReleaseActorAndSlot  64  BYTE-MATCHING
 *     FUN_08031844 470  eslesmedi (asagida ayrintili)
 *     PushSlotQueueEntry 140  RAM SEMBOLU EKSIK (asagida ayrintili)
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/band_b.c
 */

#include "gba_types.h"
#include "gba_io.h"

/* ---- 0x08031618 — 64 bayt, BYTE-MATCHING --------------------------------
 *
 * Bir aktoru serbest birakiyor: +0x1E bayrak sozunde 2 biti kuruluysa biti
 * temizleyip +0x28'deki nesne yuvasini birakiyor, sonra +0x2C'deki sahibin
 * +0x18 kimligini FUN_08030d0c'ye sorup donen yuva -1 degilse ReleaseSlot
 * cagiriyor.
 *
 * TEK ZOR NOKTA — MASKENIN GENISLIGI VE AND'IN HEDEFI:
 * ROM `ldr r0,=0xfffd / ands r0,r1 / strh r0,[r4,#30]` yaziyor; AND'in HEDEFI
 * MASKENIN yazmaci.  `actor->flags = flags & ACTOR_FLAG_CLEAR;` (ve maskeyi
 * u16 bir yerele alan her cesidi) sonucu `flags`in yazmaciyla birlestirip
 * `ands r1,r0` uretiyor — 2 bayt fark, kapanmiyor.  Olculen cozum: maske
 * 32-BIT bir yerel olacak ve AND'in hedefi O yerel olacak.  16-bit yerel ayni
 * sonucu VERMIYOR (`ands r1,r0`); genislik de belirleyici.  Ayni sonucu veren
 * esdegerler (hepsi olculdu, hepsi eslesti): u32/s32/int yerel, `next &= flags`
 * ya da `next = next & flags`, blok kapsamli ya da fonksiyon basinda bildirim.
 * Elenenler (hepsi 2 bayt fark): `flags & MASK`, `MASK & flags`, u16 maske
 * yereli, `flags &= MASK`, `flags & ~ACTOR_FLAG_HELD`, gecici sonuc degiskeni,
 * u32/s32 `flags`, maskeyi fonksiyon basina almak, held'i one almak.
 *
 * -1 karsilastirmasi `movs r0,#1 / negs r0,r0 / cmp r1,r0` olarak cikiyor;
 * Thumb'da -1 dogrudan `cmp` immediate'i olamadigi icin bu duz yazimin sonucu.
 */

#define ACTOR_FLAG_HELD    2
#define ACTOR_FLAG_CLEAR   0xFFFD       /* ~ACTOR_FLAG_HELD, 16 bit */
#define SLOT_NONE          (-1)

typedef struct Obj Obj;

typedef struct Owner {
    u8  pad00[0x18];
    u16 id;                     /* +0x18 */
} Owner;

typedef struct Actor {
    u8     pad00[0x1E];
    u16    flags;               /* +0x1E */
    u8     pad20[8];
    Obj   *held;                /* +0x28 */
    Owner *owner;               /* +0x2C */
} Actor;

extern void ReleaseObjectSlot(Obj *o);
extern s32  FUN_08030d0c(u32 id);
extern u32  ReleaseSlot(u32 index);

/* 0x08031618 */
void ReleaseActorAndSlot(Actor *actor)
{
    u16 flags;
    s32 slot;

    if (actor == 0)
        return;

    flags = actor->flags;
    if (flags & ACTOR_FLAG_HELD) {
        u32 next;

        next = ACTOR_FLAG_CLEAR;
        next = next & flags;
        actor->flags = next;
        ReleaseObjectSlot(actor->held);
    }

    slot = FUN_08030d0c(actor->owner->id);
    if (slot != SLOT_NONE)
        ReleaseSlot(slot);
}

