/* DISPCNT bit 9 kurma — 0x0803084C-0x0803085D
 *
 * REG_DISPCNT'e (0x80 << 2) bitini OR'luyor. Kardesi ClearBg1Enable
 * (0x08030860) byte-matching; bu ayri dosyada cunku eslesmiyor ve ayni
 * dosyada birakmak kardesinin bolgesini bozuyor.
 *
 * HENUZ ESLESMIYOR: bizim cikti 16 bayt, ROM 18. ROM maskeyi r3'te kurup
 * `adds r2,r3,#0` ile IKINCI bir register'a kopyaliyor; bizde o kopya hic
 * uretilmiyor:
 *     ROM  : movs r0,#128 / lsls r0,#19 / ldrh r1,[r0] /
 *            movs r3,#128 / lsls r3,#2  / adds r2,r3,#0 / orrs r1,r2 / strh
 *     bizim: movs r0,#128 / lsls r0,#2  / movs r2,#128 / lsls r2,#19 /
 *            ldrh r1,[r2] / orrs r0,r1  / strh r0,[r2]
 * Yani iki fark var: (a) sabit yuklemelerinin SIRASI ters, (b) maske
 * kopyasi eksik.
 *
 * Denenenler (ucu de 16 bayt): tek yerel; iki yerel (`mask = bit`, kural 37
 * — agbcc kopyayi birlestiriyor); once DISPCNT okuyup maskeyi sonra
 * hesaplamak (sirayi degistirmedi); maskeyi u32 yapmak.
 *
 * Kardesi ClearBg1Enable AYNI adres kalibiyla (0x80 << 19) ILK denemede
 * eslesti; fark yalnizca oradaki islemin AND, buradakinin OR olmasi. Yani
 * engel adres kurulumunda degil, maske degerinin yasam araliginda.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/set_bg1_enable.c
 */

#include "gba_types.h"

#define DISPCNT  (*(vu16 *)(0x80 << 19))
#define BG1_BIT  (0x80 << 2)

/* 0x0803084C */
void SetBg1Enable(void)
{
    u16 value;
    u16 mask;

    value = DISPCNT;
    mask = BG1_BIT;
    DISPCNT = value | mask;
}
