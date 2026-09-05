/* Kare bekleme ve iki kanca — 0x08063E58-0x08063ECF
 *
 * ROM'da bitisik uc fonksiyon.
 *
 * 0x08063E58: VBlank bekleyip bir kosul saglanana kadar donuyor, sonra
 *   bir bayragi sifirlayip 1 donuyor.
 *
 * 0x08063E7C ve 0x08063EAC: neredeyse ikiz.  gRam02025800 sifir degilse
 *   onu 28 BAYTLIK kayit tablosuna indeks olarak kullaniyor; kaydin ilk
 *   alani 6 ise kanca fonksiyonlarini cagiriyor.  Fark: 0x08063E7C iki
 *   fonksiyon cagiriyor, 0x08063EAC bir tane.
 *
 * 28 = 7*4 carpimi ROM'da `lsls #3 / subs / lsls #2` ile kuruluyor
 * (x*8 - x = x*7, sonra <<2).  Bu, agbcc'nin sabit carpim kalibi.
 *
 * Kural 35: `pop {r0}; bx r0` -> void.  0x08063E58 ise `pop {r1}; bx r1`
 * kullaniyor ve r0'da 1 tasiyor -> DEGER donduruyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/frame_wait_and_hooks.c
 */

#include "gba_types.h"

#define RECORD_MATCH 6

typedef struct Record {
    u32 kind;                   /* +0x00 */
    u8  pad04[24];
} Record;                       /* 28 bayt */

extern void VBlankIntrWait(void);
extern void FUN_08006210(void);
extern u32  FUN_08004100(void);
extern void FUN_08031328(void);
extern void FUN_080125b4(void);
extern void FUN_08012618(void);

extern u32    gRam02025800;
extern u8     gRam02035B1C;
extern Record gRom08852A2C[];

/* 0x08063E58 */
u32 FUN_08063e58(void)
{
    do {
        VBlankIntrWait();
        FUN_08006210();
    } while (FUN_08004100() == 0);

    gRam02035B1C = 0;
    return 1;
}

/* 0x08063E7C */
void FUN_08063e7c(void)
{
    u32 index;

    index = gRam02025800;
    if (index != 0 && gRom08852A2C[index].kind == RECORD_MATCH) {
        FUN_08031328();
        FUN_080125b4();
    }
}

/* 0x08063EAC */
void FUN_08063eac(void)
{
    u32 index;

    index = gRam02025800;
    if (index != 0 && gRom08852A2C[index].kind == RECORD_MATCH)
        FUN_08012618();
}
