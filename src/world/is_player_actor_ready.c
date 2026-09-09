/* Test player actor readiness — 0x0803C62C-0x0803C695
 *
 * Select player 1's block (gRam02000F10) when gGameState[12] is zero, otherwise
 * the block pointed to by gSessionPtr. Pass its actor at +0 to FUN_08061fbc;
 * return 1 if nonzero. Otherwise repeat the selection and return whether
 * mode +0x30 in the actor's +0x18 record equals 2. Rule 48: materialize the
 * condition in ok first, matching the ROM's movs r1,#0 / movs r1,#1.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/is_player_actor_ready.c
 */

#include "gba_types.h"
typedef struct Detail { u8 pad00[0x30]; u8 mode; } Detail;
typedef struct Actor { u8 pad00[24]; Detail *detail; } Actor;
typedef struct Sel { Actor *actor; } Sel;
extern u8   gGameState[];
extern Sel *gSessionPtr;
#include "ram_symbols.h"   /* gRam02000F10: u8[] view required by the consistency check */
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
