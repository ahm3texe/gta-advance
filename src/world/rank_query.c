/* Kayit alani sorgusu — 0x08066D54-0x08066ECD
 *
 * Ikinci parametre bir kayit yapisina isaretci, birincisi 0..35 arasi bir
 * alan kimligi. Kimlige gore o kaydin tek bir alanini okuyup donduruyor;
 * aralik disi kimlik icin 0.
 *
 * 36 dal atlama tablosuna (0x08066D6C, fonksiyonun kendi literal havuzu)
 * ceviriliyor: `cmp r0,#35 / bls` + `lsls r0,#2 / ldr pc-goreli taban /
 * ldr / mov pc,r0`. Aralik 0..35 bitisik oldugu icin agbcc karsilastirma
 * agaci degil tablo uretiyor; case gövdeleri ROM'da KAYNAK SIRASINDA
 * diziliyor, bu yuzden case'ler tablo sirasiyla degil ROM blok sirasiyla
 * yazildi (case 2..7, sonra 1, sonra 8..20, 22..25, 21, 26..28, 29..34,
 * 0, 35).
 *
 * Yapi bump_rank_counter.c / link_state_step.c'deki gSaveBuffer'in
 * +0x50'sinden baslayan kayit; oradaki alanlar burada su karsiliklara
 * geliyor:
 *   gSaveBuffer +0x70 sozcugu  == RecordData +0x20  (stepA/B/C, 5'er bit)
 *   gSaveBuffer +0x7E yarisozu == RecordData +0x2E  (rankA/B/C, 5'er bit)
 *
 * ONEMLI: +0x2C'deki kap u16 DEGIL u32 olmali. `unk2C_11` alani 11-16
 * bitlerinde, yani 0x2D/0x2E bayt sinirini asiyor; ne 0x2C'deki ne de
 * 0x2E'deki yarisoz alani kapsiyor, bu yuzden agbcc tam sozcuk okuyor
 * (ROM: "ldr r0,[r2,#44] / lsls #15 / lsrs #26"). rankA/B/C bu kabin
 * 17-21, 22-26, 27-31 bitleri; agbcc her biri icin alani KAPSAYAN EN DAR
 * erisimi seciyor:
 *   17-21 -> 0x2E bayti          (ldrb, lsls #26 / lsrs #27)
 *   22-26 -> 0x2E yarisozu       (ldrh, bayt sinirini asiyor)
 *   27-31 -> 0x2F baytinin tepesi(ldrb, sadece lsrs #3)
 * bump_rank_counter.c ayni bitleri +0x7E'de u16 kap olarak yaziyor; iki
 * tarif ayni bit yerlesimini veriyor, orada 6 bitlik alan okunmadigi icin
 * u16 yetiyordu.
 *
 * 0x20 ve 0x2C'deki 8 ve 4 bitlik alanlar tek komut ile cikiyor:
 *   unk20_00 (0-7 bit)  -> duz ldrb, kaydirma yok
 *   unk2C_00 (0-3 bit)  -> ldrb + lsls #28 / lsrs #28
 *
 * Bayt yuklemelerinde 0x20 ve uzeri ofsetler `adds r0,r2,#0 / adds r0,#N /
 * ldrb r0,[r0,#0]` seklinde: Thumb ldrb imm5 en fazla 31'e kadar gidiyor.
 * ldrh imm5*2 oldugu icin 0x32/0x2E gibi ofsetler dogrudan yukleniyor.
 * Yani bu uc komutluk kaliplar kaynaktaki bir tuhaflik degil, ofsetin
 * dogru olmasinin sonucu.
 *
 * Anahtar degiskeni ISARETSIZ (ROM: bls).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/rank_query.c
 */

#include "gba_types.h"

typedef struct RecordData {
    /* 0x00 */ u16 unk00;
    /* 0x02 */ u16 unk02;
    /* 0x04 */ u8  unk04;
    /* 0x05 */ u8  unk05;
    /* 0x06 */ u8  unk06;
    /* 0x07 */ u8  unk07;
    /* 0x08 */ u8  slots[8];
    /* 0x10 */ u8  unk10[4];
    /* 0x14 */ u16 unk14;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u16 unk18;
    /* 0x1A */ u16 unk1A;
    /* 0x1C */ u16 unk1C;
    /* 0x1E */ u16 unk1E;
    /* 0x20 */ u32 unk20_00 : 8;    /* bit  0-7  */
               u32 stepA    : 5;    /* bit  8-12 */
               u32 stepB    : 5;    /* bit 13-17 */
               u32 stepC    : 5;    /* bit 18-22 */
               u32 unk20_23 : 9;    /* bit 23-31 */
    /* 0x24 */ u16 unk24;
    /* 0x26 */ u16 unk26;
    /* 0x28 */ u16 unk28;
    /* 0x2A */ u16 unk2A;
    /* 0x2C */ u32 unk2C_00 : 4;    /* bit  0-3  */
               u32 unk2C_04 : 7;    /* bit  4-10 */
               u32 unk2C_11 : 6;    /* bit 11-16 */
               u32 rankA    : 5;    /* bit 17-21 */
               u32 rankB    : 5;    /* bit 22-26 */
               u32 rankC    : 5;    /* bit 27-31 */
    /* 0x30 */ u8  unk30;
    /* 0x31 */ u8  unk31;
    /* 0x32 */ u16 unk32;
    /* 0x34 */ u8  unk34;
} RecordData;

/* 0x08066D54 */
u32 GetRecordField(u32 id, RecordData *rec)
{
    switch (id) {
    case 2:  return rec->unk00;
    case 3:  return rec->unk02;
    case 4:  return rec->unk04;
    case 5:  return rec->unk05;
    case 6:  return rec->unk06;
    case 7:  return rec->unk07;
    case 1:  return rec->unk32;
    case 8:  return rec->unk2A;
    case 9:  return rec->unk18;
    case 10: return rec->unk1A;
    case 11: return rec->unk14;
    case 12: return rec->unk16;
    case 13: return rec->unk1E;
    case 14: return rec->unk1C;
    case 15: return rec->unk34;
    case 16: return rec->unk24;
    case 17: return rec->unk26;
    case 18: return rec->unk2C_11;
    case 19: return rec->unk20_00;
    case 20: return rec->unk28;
    case 22: return rec->unk30;
    case 23: return rec->stepA;
    case 24: return rec->stepB;
    case 25: return rec->stepC;
    case 21: return rec->unk2C_00;
    case 26: return rec->rankA;
    case 27: return rec->rankB;
    case 28: return rec->rankC;
    case 29: return rec->slots[0];
    case 30: return rec->slots[1];
    case 31: return rec->slots[2];
    case 32: return rec->slots[3];
    case 33: return rec->slots[4];
    case 34: return rec->slots[5];
    case 0:  return rec->slots[6];
    case 35: return rec->slots[7];
    }
    return 0;
}
