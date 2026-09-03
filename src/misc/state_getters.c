/* Kucuk durum erisimcileri — 0x08037F1C-0x08037F6F
 *
 * Yedi ozdes getter; her biri ayri bir global u32 okuyor. Globallerin
 * anlami henuz bilinmiyor, adlari adreslerinden turetildi.
 *
 * 0x08037F4C'deki fonksiyonu Ghidra'nin fonksiyon haritasi kacirmisti;
 * bu blok yazilirken bulundu.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/misc/state_getters.c
 */

#include "gba_types.h"

extern u32 gUnk0202F2C0;
extern u32 gUnk0202F310;
extern u32 gUnk02028270;
extern u32 gUnk020282A0;
extern u32 gUnk0202F2D0;
extern u32 gUnk02028290;
extern u32 gUnk02028280;

/* 0x08037F1C */
u32 GetUnk0202F2C0(void) { return gUnk0202F2C0; }

/* 0x08037F28 */
u32 GetUnk0202F310(void) { return gUnk0202F310; }

/* 0x08037F34 */
u32 GetUnk02028270(void) { return gUnk02028270; }

/* 0x08037F40 */
u32 GetUnk020282A0(void) { return gUnk020282A0; }

/* 0x08037F4C — Ghidra bu fonksiyonu kacirmisti */
u32 GetUnk0202F2D0(void) { return gUnk0202F2D0; }

/* 0x08037F58 */
u32 GetUnk02028290(void) { return gUnk02028290; }

/* 0x08037F64 */
u32 GetUnk02028280(void) { return gUnk02028280; }
