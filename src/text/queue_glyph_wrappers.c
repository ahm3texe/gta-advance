/* Ikiz sarmalayici — 0x08063E38-0x08063E57
 *
 * Iki fonksiyon da ikinci parametresini u16'ya kirpip FUN_080638a0'a
 * ILK arguman olarak geciyor; fark yalnizca ikinci argumanda:
 * 0x08063E38 sifir, 0x08063E48 bir geciyor.
 *
 * Ilk parametre (r0) KULLANILMIYOR; ROM onu hic okumuyor.
 *
 * DONUS: ROM `pop {r1}; bx r1` yapiyor, `pop {r0}; bx r0` DEGIL.  Yani r0
 * korunuyor -> bu sarmalayicilar cagirdiklari fonksiyonun donus degerini
 * GECIRIYOR, void degiller.  (Kural 35'in tersi: r0 disinda bir yazmacla
 * donuluyorsa deger donuyor demektir.)
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/queue_glyph_wrappers.c
 */

#include "gba_types.h"

extern u32 FUN_080638a0(u16 value, u32 flag);

/* 0x08063E38 */
u32 ForwardU16WithFlag0(s32 unused, u16 value)
{
    return FUN_080638a0(value, 0);
}

/* 0x08063E48 */
u32 ForwardU16WithFlag1(s32 unused, u16 value)
{
    return FUN_080638a0(value, 1);
}
