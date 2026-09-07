/* Gorev zamanlayicisi adimi — 0x080507F4-0x08050915 (290 bayt)
 *
 * DURUM: 129/138 komut, YAKIN ISKA (eslesmiyor). Boyut tutuyor.
 *
 * gRam02030330 blogu: +0x34 kuruluysa +0x38 sayaci artar; oyuncu 1
 * nesnesi FUN_0805b94c'ye gore gRam02030370 = 60 ya da geri sayilir;
 * +0x3C/+0x3E ve +0x3D/+0x3F cift sayaclari (ust bayt 128'den geri
 * sayip alttakini azaltir); +0x1C pozitifse gFrameDelay kadar azalir;
 * +0x2C sifir ve +0x10 == 1 ise +0x18 geri sayimi bitince +0x10 = 0,
 * FUN_0802a5b4(0,1), +0x18 = 1200, +0x10 <= 0 ise FUN_0805063c;
 * durum (+0x08) 1: (+0x04 sifirsa GetActiveSlotValue) FUN_080501c8,
 * 2: FUN_0805063c, 3: +0 = 0, +8 = 1, +0xC = 25, +0x28 = 2, +0x30 = 0.
 *
 * OLCULEN: blok bir yerel isaretciyle DEGIL dogrudan global uyeleriyle
 * yazilmali (ROM adresi uc kez havuzdan yeniden yukluyor: 85 -> 115);
 * switch'e `case 0: break;` eklenmeli -- agac koku 2'den 1'e iniyor ve
 * `bcc default` cikiyor (115 -> 129). gFrameDelay `(*(u32*)0x03000000)`.
 *
 * KALAN 9 KOMUT: switch icin taban adresinin callee-saved r4'te tutulmasi
 * (bende r2) ve case 3'te `2` sabitinin sifirdan once r3'e alinmasi.
 * Denenen: yerel `state`, atama sirasi, sabit yerelleri, zincir atama --
 * hepsi 9-10. Kural 44 sinifi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/step_mission_timer.c
 */

#include "gba_types.h"
/* gRam02030330 sekiz eslesen dosyada Anchor olarak bildirili (tutarlilik
 * kapisi); +0x3C..+0x3F baytlari govde disinda kaldigi icin bayt
 * uzakligiyla okunuyor. */
typedef struct Anchor {
    u32 unk00;                  /* +0x00 */
    u32 unk04;                  /* +0x04 */
    u32 unk08;                  /* +0x08 */
    u32 unk0C;                  /* +0x0C = boyut */
    u32 base;                   /* +0x10 */
    u32 unk14;                  /* +0x14 */
    u32 unk18;                  /* +0x18 */
    u32 unk1C;                  /* +0x1C */
    u32 unk20;                  /* +0x20 */
    u32 unk24;                  /* +0x24 */
    u32 unk28;                  /* +0x28 */
    u32 slot;                   /* +0x2C */
    u32 unk30;                  /* +0x30 */
    u32 unk34;                  /* +0x34 */
    u32 unk38;                  /* +0x38 */
} Anchor;
typedef struct SlotA { void *obj; } SlotA;
extern Anchor  gRam02030330;
extern u8      gRam02030370;
#define gFrameDelay (*(u32 *)0x03000000)
#define M_BYTE(off) (((u8 *)&gRam02030330)[off])
extern SlotA  *SelectSlotAB(u32 which);
extern u32     FUN_0805b94c(void *obj);
extern void    FUN_0802a5b4(u32 a, u32 b);
extern void    FUN_0805063c(void);
extern u32     GetActiveSlotValue(void);
extern void    FUN_080501c8(void);
void StepMissionTimer(void)
{
    if (gRam02030330.unk34 != 0)
        gRam02030330.unk38++;
    if (FUN_0805b94c(SelectSlotAB(1)->obj) != 0)
        gRam02030370 = 60;
    else if (gRam02030370 != 0)
        gRam02030370--;
    if (M_BYTE(0x3C) != 0) {
        if (M_BYTE(0x3E) == 0 || --M_BYTE(0x3E) == 0) {
            M_BYTE(0x3C)--;
            M_BYTE(0x3E) = 128;
        }
    }
    if (M_BYTE(0x3D) != 0) {
        if (M_BYTE(0x3F) == 0 || --M_BYTE(0x3F) == 0) {
            M_BYTE(0x3D)--;
            M_BYTE(0x3F) = 128;
        }
    }
    if (gRam02030330.unk1C > 0)
        gRam02030330.unk1C -= gFrameDelay;
    if (gRam02030330.slot == 0 && gRam02030330.base == 1) {
        gRam02030330.unk18 -= gFrameDelay;
        if (gRam02030330.unk18 < 0) {
            gRam02030330.base = 0;
            FUN_0802a5b4(0, 1);
            gRam02030330.unk18 = 1200;
            if (gRam02030330.base <= 0)
                FUN_0805063c();
        }
    }
    switch (gRam02030330.unk08) {
    case 0:
        break;
    case 1:
        if (gRam02030330.unk04 == 0)
            GetActiveSlotValue();
        FUN_080501c8();
        break;
    case 2:
        FUN_0805063c();
        break;
    case 3:
        gRam02030330.unk00 = 0;
        gRam02030330.unk08 = 1;
        gRam02030330.unk0C = 25;
        gRam02030330.unk28 = 2;
        gRam02030330.unk30 = 0;
        break;
    }
}
