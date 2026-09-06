/* Iki yarim sozun esitligi — 0x08030BE8-0x08030C0B  (36 bayt, BYTE-MATCHING)
 *
 * gRam02025810'un +0x04 ve +0x1C yarim sozlerini 0xFFFF ile suzup
 * karsilastiriyor. ROM taban isaretcisini ve maskeyi havuzdan yukluyor,
 * maskeyi `adds r2,r1,#0` ile kopyalayip sol tarafta kullaniyor; sag
 * tarafta maske olu oldugu icin yerinde `ands r1,r0` yapiyor.
 *
 * ROM (olculdu):
 *   ldr r0,[pc]   -> 0x02025810      ldr r1,[pc] -> 0x0000ffff
 *   adds r2,r1,#0                    <- maskenin kopyasi (sol taraf temp)
 *   ldrh r3,[r0,#4]  / ands r2,r3    <- sol taraf TAM olarak once
 *   ldrh r0,[r0,#28] / ands r1,r0    <- taban isaretcisi burada oluyor
 *   cmp r2,r1 / beq -> sona konmus `return 1` govdesi (kural 49)
 *   duserek: movs r0,#0 ; b epilog ; havuz ; movs r0,#1 ; bx lr
 *
 * ESLESMEYI SAGLAYAN SEY -- MASKE AYRI BIR YERELDE OLMALI.
 * Alanlar u16 oldugu icin `& 0xFFFF` bir sabit olarak yazilirsa agbcc onu
 * gereksiz sayip siliyor ve cikti 24 bayta iniyor (fark 35). Maske bir
 * yerel degiskene alininca AND'ler ayakta kaliyor ve boyut 36'ya oturuyor.
 *
 * IKINCI VE ASIL AYRINTI -- TANIM SIRASI (yeni olculdu):
 * Isaretci ile maskenin ILK TANIM sirasi hem havuz sirasini hem yazmac
 * dagitimini belirliyor. Isaretci once tanimlanmali.
 *   b = ...; mask = 0xFFFF;   -> r0=taban, r1=maske, havuz [0x02025810,
 *                                0x0000ffff]  => fark 0
 *   mask = 0xFFFF; b = ...;   -> r0=maske, r2=taban, havuz TERS
 *                                (0x0000ffff once) => fark 15
 * Yani onceki turdaki "36 bayt / 15 fark" sonucu yanlis bir yazim degil,
 * yalnizca ters tanim siralamasiydi. Iki satirin yerini degistirmek
 * 15 farki 0'a indirdi.
 *
 * DENENIP ELENEN YAZIMLAR:
 *  - `b->first & 0xFFFF` (sabit dogrudan): 24 bayt, fark 35. Maske atiliyor.
 *  - Alanlari u32 yapmak: 24 bayt; maske yine atildi, ustelik ldrh yerine
 *    ldr cikti.
 *  - Iki ayri maske yereli (m1, m2): 36 bayt / 17 fark. ROM tek maske
 *    yerelinden kopya uretiyor, iki ayri yerelden degil.
 *  - `mask` once tanimlanmis her yazim (bildirim sirasi da, atama sirasi
 *    da): 36 bayt / 15 fark. Tekrar denemeyin.
 * NOT: maskenin tipi u32 veya u16 olmasi fark etmiyor -- her ikisi de
 * fark 0 veriyor (olculdu). u32 birakildi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/halves_equal.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define HALF_MASK 0xFFFF

typedef struct Block {
    u8  pad00[4];
    u16 first;                  /* +0x04 */
    u8  pad06[22];
    u16 second;                 /* +0x1C */
} Block;

/* 0x08030BE8 */
u32 HalvesEqual(void)
{
    Block *b;
    u32 mask;

    /* gRam02025810 paylasilan ham depolama (include/ram_symbols.h);
       her ceviri birimi kendi gorunumune YERELDE cast eder. */
    b = (Block *)gRam02025810;
    mask = HALF_MASK;           /* SIRA ONEMLI: isaretciden SONRA */
    if ((b->first & mask) == (b->second & mask))
        return 1;
    return 0;
}
