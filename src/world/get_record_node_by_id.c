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

/* ---- 0x08031318 — 14 bayt, BYTE-MATCHING --------------------------------
 *
 * 16 bitlik kimligi GetOrCreateRecordNode'a iletip sonucunu donduruyor.
 *
 * IKI OLCUM:
 *  - `pop {r1}; bx r1` (r0 DEGIL) -> donus degeri kullaniliyor, yani fonksiyon
 *    cagrinin sonucunu dondurur.  Kural 35'in tersi yonu.
 *  - Bastaki `lsls r0,#16 / lsrs r0,#16` cifti PARAMETRENIN u16 olmasindan
 *    geliyor.  `id & 0xFFFF` yazimi bunun yerine havuzdan 0xFFFF yukleyip
 *    `ands` uretiyor (olculdu: 20 bayt, 3/9 komut).
 */

typedef struct Node Node;

extern Node *GetOrCreateRecordNode(s32 id);

/* 0x08031318 */
Node *GetRecordNodeById(u16 id)
{
    return GetOrCreateRecordNode(id);
}

