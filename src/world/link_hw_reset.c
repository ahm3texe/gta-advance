/* Baglanti donanimini sifirlar — 0x08066A54-0x08066AA7
 *
 * Timer3 ve seri kesmelerini (IE bit 6-7) kapatiyor, SIOCNT'yi yeniden
 * kuruyor, Timer3'u tek 32-bit yazimla durdurup yeniden yukleme degerini
 * yaziyor, bekleyen IF bitlerini onayliyor ve baglanti kaydinin +0x06
 * alanini siliyor.
 *
 * IE yazimlari IME kapaliyken yapiliyor; sira ROM'dan okundu.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/link_hw_reset.c
 */

#include "gba_types.h"
#include "gba_io.h"

#define IE_KEEP_MASK   0xFF3F   /* Timer3 ve seri disindaki her sey */
#define IF_ACK_LINK    0xC0     /* Timer3 | seri */
#define SIOCNT_RESET   0x2003
#define TM3_RELOAD     0x0000ABFB

/* src/world/comm_flag.c ile ayni tur: iki dosya ayni sembolu farkli
 * struct'la bildirirse tutarlilik denetimi hakli olarak duruyor. */
typedef struct CommBlock {
    u8  byte0;
    u8  pad01[5];
    u8  byte6;                  /* +0x06 */
} CommBlock;

extern CommBlock *gRam02036338;

/* 0x08066A54 */
void ResetLinkHardware(void)
{
    REG_IME = 0;
    REG_IE = REG_IE & IE_KEEP_MASK;
    REG_IME = 1;

    REG_SIOCNT = SIOCNT_RESET;
    REG_TM3CNT = TM3_RELOAD;
    REG_IF = IF_ACK_LINK;

    gRam02036338->byte6 = 0;
}
