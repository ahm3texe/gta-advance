/* One step of the mission timer — 0x080507F4-0x08050915 (290 bytes)
 *
 * STATUS: 129/138 instructions, A NEAR MISS (does not match).  The size is
 * right.
 *
 * The gRam02030330 block: if +0x34 is set the +0x38 counter increments;
 * according to FUN_0805b94c on the player 1 object, gRam02030370 is either set
 * to 60 or counted down; the +0x3C/+0x3E and +0x3D/+0x3F counter pairs (the
 * high byte counts down from 128 and decrements the low one); if +0x1C is
 * positive it decreases by gFrameDelay; if +0x2C is zero and +0x10 == 1, then
 * when the +0x18 countdown finishes +0x10 = 0, FUN_0802a5b4(0,1), +0x18 = 1200,
 * and if +0x10 <= 0, FUN_0805063c; state (+0x08) 1: (GetActiveSlotValue when
 * +0x04 is zero) FUN_080501c8, 2: FUN_0805063c, 3: +0 = 0, +8 = 1, +0xC = 25,
 * +0x28 = 2, +0x30 = 0.
 *
 * MEASURED: the block must be written through the global's members directly,
 * NOT through a local pointer (the ROM reloads the address from the pool three
 * times: 85 -> 115); `case 0: break;` must be added to the switch -- the tree
 * root drops from 2 to 1 and `bcc default` comes out (115 -> 129).
 * gFrameDelay is `(*(u32*)0x03000000)`.
 *
 * THE REMAINING 9 INSTRUCTIONS: the switch's base address being kept in the
 * callee-saved r4 (r2 here), and the constant `2` in case 3 being taken into
 * r3 before the zero.  Tried: a local `state`, the assignment order, constant
 * locals, chained assignment -- all 9-10.  A rule 44 class.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/step_mission_timer.c
 */

#include "gba_types.h"
/* gRam02030330 is declared as Anchor in eight matching files (the consistency
 * gate); because bytes +0x3C..+0x3F fall outside that body, they are read at a
 * byte offset. */
typedef struct Anchor {
    u32 unk00;                  /* +0x00 */
    u32 unk04;                  /* +0x04 */
    u32 unk08;                  /* +0x08 */
    u32 unk0C;                  /* +0x0C = size */
    u32 base;                   /* +0x10 */
    u32 unk14;                  /* +0x14 */
    u32 unk18;                  /* +0x18 */
    u32 unk1C;                  /* +0x1C */
    u32 unk20;                  /* +0x20 */
    u32 unk24;                  /* +0x24 */
    u32 unk28;                  /* +0x28 */
    u32 slot;                   /* +0x2C */
    u32 unk30;                  /* +0x30 */
    u32 unk34;                  /* +0x34 */
    u32 unk38;                  /* +0x38 */
} Anchor;
typedef struct SlotA { void *obj; } SlotA;
extern Anchor  gRam02030330;
extern u8      gRam02030370;
#define gFrameDelay (*(u32 *)0x03000000)
#define M_BYTE(off) (((u8 *)&gRam02030330)[off])
extern SlotA  *SelectSlotAB(u32 which);
extern u32     FUN_0805b94c(void *obj);
extern void    FUN_0802a5b4(u32 a, u32 b);
extern void    FUN_0805063c(void);
extern u32     GetActiveSlotValue(void);
extern void    FUN_080501c8(void);
void StepMissionTimer(void)
{
    if (gRam02030330.unk34 != 0)
        gRam02030330.unk38++;
    if (FUN_0805b94c(SelectSlotAB(1)->obj) != 0)
        gRam02030370 = 60;
    else if (gRam02030370 != 0)
        gRam02030370--;
    if (M_BYTE(0x3C) != 0) {
        if (M_BYTE(0x3E) == 0 || --M_BYTE(0x3E) == 0) {
            M_BYTE(0x3C)--;
            M_BYTE(0x3E) = 128;
        }
    }
    if (M_BYTE(0x3D) != 0) {
        if (M_BYTE(0x3F) == 0 || --M_BYTE(0x3F) == 0) {
            M_BYTE(0x3D)--;
            M_BYTE(0x3F) = 128;
        }
    }
    if (gRam02030330.unk1C > 0)
        gRam02030330.unk1C -= gFrameDelay;
    if (gRam02030330.slot == 0 && gRam02030330.base == 1) {
        gRam02030330.unk18 -= gFrameDelay;
        if (gRam02030330.unk18 < 0) {
            gRam02030330.base = 0;
            FUN_0802a5b4(0, 1);
            gRam02030330.unk18 = 1200;
            if (gRam02030330.base <= 0)
                FUN_0805063c();
        }
    }
    switch (gRam02030330.unk08) {
    case 0:
        break;
    case 1:
        if (gRam02030330.unk04 == 0)
            GetActiveSlotValue();
        FUN_080501c8();
        break;
    case 2:
        FUN_0805063c();
        break;
    case 3:
        gRam02030330.unk00 = 0;
        gRam02030330.unk08 = 1;
        gRam02030330.unk0C = 25;
        gRam02030330.unk28 = 2;
        gRam02030330.unk30 = 0;
        break;
    }
}
