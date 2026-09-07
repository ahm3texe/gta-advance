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

/* ---- 0x08031414 — 132 bayt, BYTE-MATCHING -------------------------------
 *
 * Isaretli 16.16 sabit noktali bir buyuklugu parcali-dogrusal bir egriyle
 * esliyor.  Girdinin mutlak degeri alinip bes banda bolunuyor; her bant
 * `((v - bant_basi) * egim >> 16) + taban` biciminde ve bantlar sinirlarda
 * surekli (20 / 60 / 100 / 200).
 *
 * IKI OLCUM:
 *  - MUTLAK DEGER YAZIMI.  ROM `adds r1,r0,#0 / cmp r1,#0 / bge / negs r0,r1 /
 *    adds r1,r0,#0` uretiyor: negatiflemenin HEDEFI parametrenin yazmaci, sonra
 *    geri kopya.  Duz `if (value < 0) value = -value; v = value;` yazimi tek
 *    `negs r1,r1` veriyor (71/132 bayt fark).  Asagidaki uc satirlik bicim —
 *    v'yi once kopyala, negatiflemeyi PARAMETREYE yaz, v'yi tazele — ROM'un
 *    besli dizisini birebir uretiyor.  Elenen esdegerler (hepsi 71 fark):
 *    ucluk operatoru, `0 - value`, tek degiskenli yazim, iki gecici degisken.
 *  - SON IKI BANDIN SIRASI.  `if (v > BAND4_END) return ...+200; return ...+100;`
 *    yazimi agbcc'de ters cevriliyor ve +100 govdesi one geciyor (30 bayt fark).
 *    ROM'un `ble` + govde sirasi ancak `if (v <= BAND4_END) return ...+100;`
 *    ile cikiyor — yani ilk uc bantla AYNI kalip.  `else` eklemek etkisiz.
 *  - Egimler kaynakta duz carpim: agbcc 10/20/40'i `(v<<2)+v` + kaydirma olarak
 *    sentezliyor, 50'yi ise `movs r0,#50 / muls r0,r1` yapiyor (kural 53).
 */

#define FRAC_BITS  16
#define BAND1_END  0x20000
#define BAND2_END  0x40000
#define BAND3_END  0x50000
#define BAND4_END  0x70000
#define SLOPE1     10
#define SLOPE2     20
#define SLOPE3     40
#define SLOPE4     50
#define SLOPE5     40
#define BASE2      20
#define BASE3      60
#define BASE4      100
#define BASE5      200

/* 0x08031414 */
s32 ScaleMagnitude(s32 value)
{
    s32 v;

    v = value;
    if (v < 0)
        value = -v;
    v = value;

    if (v <= BAND1_END)
        return (v * SLOPE1) >> FRAC_BITS;
    if (v <= BAND2_END)
        return (((v - BAND1_END) * SLOPE2) >> FRAC_BITS) + BASE2;
    if (v <= BAND3_END)
        return (((v - BAND2_END) * SLOPE3) >> FRAC_BITS) + BASE3;
    if (v <= BAND4_END)
        return (((v - BAND3_END) * SLOPE4) >> FRAC_BITS) + BASE4;
    return (((v - BAND4_END) * SLOPE5) >> FRAC_BITS) + BASE5;
}

