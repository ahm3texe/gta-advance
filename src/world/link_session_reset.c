/* Baglanti oturumunu bastan kurar — 0x08066144-0x0806620B
 *
 * Once baglanti blogunu kuruyor, sonra sekiz ayri calisma tamponunu
 * sifirliyor (boyutlar Memset cagrilarindan OLCULDU: 8, 64, 64, 32, 32,
 * 32, 32, 32) ve oturum durumu alanlarini baslangic degerlerine cekiyor.
 *
 * DURUM: PARK — 200/200 boyut, 8 bayt fark, tek komut kalibi.
 *
 * ROM iki bayt hedefinin adresini AYNI ANDA iki yazmacta tutup tek sifirla
 * ard arda yaziyor:
 *     ldr r2,=0x0203632C / ldr r1,=0x02036320 / movs r0,#0 / strb / strb
 * Bizim derleme her adresi kendi yaziminin hemen oncesinde r0'a yukluyor.
 *
 * Olculdu (dump_alloc --function ResetLinkSession): iki adres sabiti ayri
 * pseudo (22 ve 24), ikisi de refs 2 / omur 4 / oncelik 0,500 ve ikisi de
 * YEREL dagitici tarafindan r0'a veriliyor. Global yarisa girmiyorlar,
 * yani kural 50'nin oncelik kolu burada islemiyor.
 *
 * ELENEN YAZIMLAR: iki atamanin sirasini cevirmek (10 -> 8 bayt, en iyisi
 * bu), gGameState.word00'i one almak (46), gRam02036328'i basa almak (57),
 * ortak bir yerel sifir degiskeni (39).
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
extern u8  gRam02000420[];
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

    gRam0203632C = 0;
    gRam02036320 = 0;

    gGameState.word00 = 0;
    gRam0200048C      = 0;
    gVBlankEnabled    = LINK_STATE_READY;
    gGameState.half04 = 0;

    gRam02036330.half02 = 0;
    gRam020110B8        = 0;
    gRam02036328        = 0;
}
