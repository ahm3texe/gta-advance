/* Applying the launch velocity — 0x08064DFC-0x08064F21 (294 bytes)
 *
 * STATUS: 119/143 instructions, A NEAR MISS (does not match).  292/294 bytes:
 * the ROM is 2 bytes longer.
 *
 * WHAT IT DOES: writes the mode byte to the slot (+0x25); clamps forward and
 * backward using the speed and the 16.16 angle (angle >>16) (forward: return
 * if the angle is 1..0x2FF, backward: return if the angle is > 0x100); if
 * forward, it subtracts the angle from 0x400, takes a value from the
 * gRom08CA30D8 sine table (<<2, >>8), multiplies it by the absolute value of
 * the speed (>>8) and writes it <<8 to +0x28; +0x40 = 0; if the speed is
 * negative it negates +0x124; +0x3C = half of +0x124 (with the opposite sign);
 * if |+0x3C| < (+0xE0 - +0xD4)/4 it zeroes it; makes a selection from the
 * +0x18C..+0x19C chain and adds it to +0x120; scales the speed with
 * ScaleByDistanceBand; sound 472.
 *
 * THE REMAINING 24 INSTRUCTIONS, THREE CLUSTERS (all register allocation /
 * read order):
 *  1. the sine r0 / absolute-speed r1 roles are swapped (r1 / r0 here).
 *  2. the +0x3C ternary is computed directly into r0 and stored in the ROM,
 *     and the field is RE-READ for the comparison (`ldr r1,[r4,#60]`); here
 *     the value stays in r2.  Tried: a local v, the ternary directly, a `*w`
 *     alias pointer (26), computing d first -- none of them.
 *  3. the address constants of the +0x18C/+0x190/+0x194 triple: the ROM
 *     derives +0x194 from the +0x18C constant (+8), here from +0x190 (+4).
 *     Tried: separate locals p0..p4 (118), `q = &b->unk18C; q[i]` (98),
 *     doing the shifts inline (119, the best).
 * These three clusters are a rule 44 class (register allocation); there is no
 * known source lever.  Whoever tries next: look at rules 51/54 in
 * docs/COMPILER.md and start with cluster 2.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/apply_launch_velocity.c
 */

#include "gba_types.h"
#define ANGLE_MASK  0x3FF
#define FULL_TURN   0x400
#define FWD_MAX     0x2FF
#define BACK_MAX    0x100
#define SOUND_ID    472
typedef struct Inner { u8 pad00[0x38]; s32 band; } Inner;
typedef struct Link { u8 pad00[16]; Inner *inner; } Link;
typedef struct Body {
    u8 pad00[0x18]; s32 speed; u8 pad1c[0xC]; s32 unk28; u8 pad2c[0x10];
    s32 unk3C; s32 unk40; s32 angle; u8 pad48[0x1C]; void *entity; u8 pad68[0x6C];
    s32 unkD4; u8 padd8[8]; s32 unkE0; u8 pade4[0x3C]; s32 unk120; s32 unk124;
    u8 pad128[0x64]; s32 unk18C; s32 unk190; s32 unk194; s32 unk198; s32 unk19C;
    u8 pad1a0[0x10]; Link *link;
} Body;
extern const s16 gRom08CA30D8[];
extern u32   GetOwnerSlot(void *entity);
extern u8   *SelectSlotAB(u32 which);
extern s32   ScaleByDistanceBand(s32 value, s32 band);
extern void  FUN_08035168(u32 id);
void ApplyLaunchVelocity(Body *b, u32 mode)
{
    u8 *slot; s32 speed; s32 sp; s32 ang; s32 sine; s32 mag; s32 d; s32 sel; s32 p0; s32 p1; s32 p2; s32 p3; s32 p4;
    slot = SelectSlotAB(GetOwnerSlot(b->entity));
    if (slot != 0 && mode != 0)
        slot[0x25] = mode;
    speed = b->speed;
    ang = b->angle >> 16;
    sp = speed;
    if (sp > 0 && ang <= FWD_MAX && ang > 0)
        return;
    if (speed < 0 && ang > BACK_MAX)
        return;
    if (speed > 0)
        ang = FULL_TURN - ang;
    sine = gRom08CA30D8[ang & ANGLE_MASK] << 2;
    mag = speed;
    if (mag < 0)
        mag = -mag;
    b->unk28 = ((sine >> 8) * (mag >> 8)) << 8;
    b->unk40 = 0;
    if (sp < 0)
        b->unk124 = -b->unk124;
    b->unk3C = (b->unk124 < 0) ? (-b->unk124 >> 1) : -(b->unk124 >> 1);
    d = (b->unkE0 - b->unkD4) >> 2;
    if (b->unk3C > -d && b->unk3C < d)
        b->unk3C = 0;
    p0 = b->unk18C;
    p1 = b->unk190;
    p2 = b->unk194;
    sel = p0;
    if (p2 > sel) sel = p1;
    p3 = b->unk198;
    if (p3 > sel) sel = p2;
    p4 = b->unk19C;
    if (p4 > sel) sel = p3;
    b->unk120 += sel;
    b->speed = ScaleByDistanceBand(b->speed, b->link->inner->band);
    FUN_08035168(SOUND_ID);
}
