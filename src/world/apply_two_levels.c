/* Clamping and applying two levels — 0x0803C0E4-0x0803C177
 *
 * If bit 0 of the flag is set it writes the value to gRam02000F10 +0x0C; if
 * bit 1 is set (and the game state is active) to gRam02001140 +0x0C.  In both
 * cases the value is clamped to [0, 0x10000], and if the selector holds the
 * right value the scaled form is passed to FUN_08030A60.
 *
 * AN IMPORTANT ASYMMETRY: the selector (gSlotSelector) is read SIGNED in the
 * FIRST block (`ldrsh`, a != 0 test) and UNSIGNED in the SECOND (`ldrh`, a
 * == 1 test).  Writing the two the same way produces a different load
 * instruction.
 *
 * The upper bound is built as `0x80 << 9`; a plain 0x10000 would have produced
 * a pool load.
 *
 * gRam02000F10 and gRam02001140 are shared raw storage
 * (include/ram_symbols.h); they are cast in a local.
 *
 * Rule 35: `pop {r0}; bx r0` indicates a void return type.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/apply_two_levels.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define FLAG_FIRST  1
#define FLAG_SECOND 2
#define LEVEL_MAX   (0x80 << 9)
#define SCALE       100

typedef struct Level {
    u8  pad00[12];
    s32 value;                  /* +0x0C */
} Level;

typedef struct GameState {
    u8 pad00[12];
    u8 flag;                    /* +0x0C */
} GameState;

extern u16 gSlotSelector;

/* The same symbol is read with TWO DIFFERENT SIGNEDNESSES in this function:
   `ldrsh` (signed) in the first block, `ldrh` (unsigned) in the second.  An
   inline `(s16)` cast and a volatile read are BOTH DISCARDED BY AGBCC (the
   sign is treated as irrelevant in a comparison against zero, and 144 bytes
   come out); a separate macro declaration survives and produces the
   `movs r2,#0` + `ldrsh r0,[r0,r2]` pair. */
#define gSlotSelectorSigned (*(s16 *)&gSlotSelector)
extern GameState gGameState;

extern void FUN_08030a60(s32 scaled, u32 arg);

/* 0x0803C0E4 */
void ApplyTwoLevels(s32 value, u32 flags)
{
    Level *level;

    if (flags & FLAG_FIRST) {
        level = (Level *)gRam02000F10;
        level->value = value;
        if (value > LEVEL_MAX)
            level->value = LEVEL_MAX;
        if (level->value < 0)
            level->value = 0;
        if (gSlotSelectorSigned == 0)
            FUN_08030a60((SCALE * level->value) >> 16, 1);
    }

    if (flags & FLAG_SECOND) {
        if (gGameState.flag != 0) {
            level = (Level *)gRam02001140;
            level->value = value;
            if (value > LEVEL_MAX)
                level->value = LEVEL_MAX;
            if (level->value < 0)
                level->value = 0;
            if (gSlotSelector == 1)
                FUN_08030a60((SCALE * level->value) >> 16, 1);
        }
    }
}
