/* Iki yarim sozun esitligi — 0x08030BE8-0x08030C0B
 *
 * gRam02025810'un +0x04 ve +0x1C yarim sozlerini 0xFFFF ile suzup
 * karsilastiriyor. ROM maskeyi havuzdan yukleyip `adds r2,r1,#0` ile
 * kopyaliyor; en sade ifade bicimi bunu dogal olarak uretiyor
 * (bkz. src/world/set_bg1_enable.c -- yerel eklemek kopyayi bastirir).
 *
 * HENUZ ESLESMIYOR: bizim 24 bayt, ROM 36. Maskem GEREKSIZ sayilip
 * atiliyor: alanlar u16 oldugu icin `& 0xFFFF` bir sey degistirmiyor ve
 * agbcc onu siliyor; ROM ise maskeyi havuzdan yukleyip `adds r2,r1,#0`
 * ile kopyalayarak gercekten uyguluyor.
 *
 * Varyant taramasi: alanlari u32 yapmak 24 (maske yine atildi, ustelik
 * ldrh yerine ldr cikardi); maskeyi ayri yerele almak 36 bayt / 15 fark
 * (BOYUT DOGRU, en yakin); iki ayri maske yereli 36 / 17.
 *
 * Yani boyutu tutturmak maskeyi yerelde tutmakla mumkun ama register
 * dagitimi henuz oturmuyor. Sonraki tur oradan devam etmeli.
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

    /* gRam02025810 paylasilan ham depolama (include/ram_symbols.h);
       her ceviri birimi kendi gorunumune YERELDE cast eder. */
    b = (Block *)gRam02025810;
    if ((b->first & HALF_MASK) == (b->second & HALF_MASK))
        return 1;
    return 0;
}
