/* Baglanti oturumunu bastan kurar — 0x08066144-0x0806620B
 *
 * Once baglanti blogunu kuruyor, sonra sekiz ayri calisma tamponunu
 * sifirliyor (boyutlar Memset cagrilarindan OLCULDU: 8, 64, 64, 32, 32,
 * 32, 32, 32) ve oturum durumu alanlarini baslangic degerlerine cekiyor.
 *
 * ZINCIRLI ATAMA SART (kural 52). Iki bayt hedefi ayri ifadelerle
 * yazilinca yerel dagitici her adresi sirayla ayni yazmaca koyuyor ve
 * ROM'dan 8 bayt sapiyoruz; `a = (b = 0)` bicimi tek sifir degeri uretip
 * iki adresi ES ZAMANLI canli tutuyor, ROM da oyle yapiyor.
 * Bu bicimi permuter buldu (build/permuter/ResetLinkSession).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/link_session_reset.c
 */

#include "gba_types.h"
#include "comm_block.h"
#include "game_state.h"

#define LINK_STATE_READY 1

typedef struct LinkCounters {
    u8  pad00[2];
    u16 half02;                 /* +0x02 */
    u8  pad04[4];
} LinkCounters;

extern CommBlock   gLinkBlock;
extern LinkCounters gRam02036330;
extern u16 gRam02000420[];
extern u8  gRam02000230[];
extern u8  gRam020003C0[];
extern u8  gRam02000E80[];
extern u8  gRam02000100[];
extern u8  gRam02000400[];
extern u8  gRam02000140[];
extern u8  gRam02036320;
extern u8  gRam0203632C;
extern u8  gRam02036328;
extern u16 gRam0200048C;
extern u32 gRam020110B8;
extern u16 gVBlankEnabled;

extern void InitLinkBlock(CommBlock *block);
extern void Memset(void *dest, int value, u32 size);

/* 0x08066144 */
void ResetLinkSession(void)
{
    InitLinkBlock(&gLinkBlock);

    Memset(&gRam02036330, 0, 8);
    Memset(gRam02000420, 0, 64);
    Memset(gRam02000230, 0, 64);
    Memset(gRam020003C0, 0, 32);
    Memset(gRam02000E80, 0, 32);
    Memset(gRam02000100, 0, 32);
    Memset(gRam02000400, 0, 32);
    Memset(gRam02000140, 0, 32);

    gRam0203632C = (gRam02036320 = 0);

    gGameState.word00 = 0;
    gRam0200048C      = 0;
    gVBlankEnabled    = LINK_STATE_READY;
    gGameState.half04 = 0;

    gRam02036330.half02 = 0;
    gRam020110B8        = 0;
    gRam02036328        = 0;
}
