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

/* ---- 0x08031578 — 72 bayt, BYTE-MATCHING --------------------------------
 *
 * Dort VRAM karo satirini bos karo (0xF0E8) ile dolduruyor: iki dongu, her
 * biri sekiz yarim soz, isaretciler GERIYE dogru.
 *
 * src/ui/hud_fields.c'deki ClearHudFieldA ile ayni kalip:
 *  - karo sabiti DOGRUDAN store ifadesinde (HImode store -> `ldr` + `adds`
 *    kopyasi; yerele alinirsa kopya kayboluyor, MEKANIZMA 1),
 *  - sayac ARTAN yazilmali; agbcc azalan cevrime kendisi ceviriyor ve sayac
 *    ilklendirmesi tasinan sabitten SONRA yayiliyor (MEKANIZMA 2).
 * ClearHudFieldA'daki omur uzatan `a++; a--;` no-op'una burada GEREK YOK:
 * ROM'un dagilimi (a=r2, karo=r3) zaten duz yazimin urettigi dagilim.
 */

#define TILE_BLANK 0xF0E8
#define TILE_RUN   8

#define ROW_A_LEFT  ((vu16 *)0x060099BA)
#define ROW_A_RIGHT ((vu16 *)0x060099FA)
#define ROW_B_LEFT  ((vu16 *)0x06009A3A)
#define ROW_B_RIGHT ((vu16 *)0x06009A7A)

/* 0x08031578 */
void ClearHudRowsAB(void)
{
    vu16 *a;
    vu16 *b;
    s32 i;

    a = ROW_A_LEFT;
    b = ROW_A_RIGHT;
    for (i = 0; i < TILE_RUN; i++) {
        *a = TILE_BLANK;
        a--;
        *b = TILE_BLANK;
        b--;
    }

    a = ROW_B_LEFT;
    b = ROW_B_RIGHT;
    for (i = 0; i < TILE_RUN; i++) {
        *a = TILE_BLANK;
        a--;
        *b = TILE_BLANK;
        b--;
    }
}

