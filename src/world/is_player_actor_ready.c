/* Oyuncu aktoru hazir mi — 0x0803C62C-0x0803C695
 *
 * gGameState[12] sifirsa oyuncu 1 secim blogu (gRam02000F10), degilse
 * gSessionPtr'in gosterdigi blok; +0'daki aktor FUN_08061fbc'ye
 * veriliyor, sifir degilse 1. Yoksa ayni secim yeniden yapilip aktorun
 * +0x18 kaydinin +0x30 kipi 2 ise 1, degilse 0 (kural 48: kosul once
 * `ok` degiskeninde maddelesiyor, ROM'daki `movs r1,#0 / movs r1,#1`).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/is_player_actor_ready.c
 */

#include "gba_types.h"
typedef struct Detail { u8 pad00[0x30]; u8 mode; } Detail;
typedef struct Actor { u8 pad00[24]; Detail *detail; } Actor;
typedef struct Sel { Actor *actor; } Sel;
extern u8   gGameState[];
extern Sel *gSessionPtr;
#include "ram_symbols.h"   /* gRam02000F10: u8[] gorunumu, tutarlilik kapisi */
extern u32  FUN_08061fbc(Actor *actor);
u32 IsPlayerActorReady(void)
{
    Sel *sel; Actor *actor; u32 ok;
    if (gGameState[12] == 0)
        sel = (Sel *)gRam02000F10;
    else
        sel = gSessionPtr;
    if (FUN_08061fbc(sel->actor) != 0)
        return 1;
    if (gGameState[12] == 0)
        sel = (Sel *)gRam02000F10;
    else
        sel = gSessionPtr;
    actor = sel->actor;
    if (actor == 0)
        return 0;
    ok = 0;
    if (actor->detail->mode == 2)
        ok = 1;
    if (ok == 0)
        return 0;
    return 1;
}
