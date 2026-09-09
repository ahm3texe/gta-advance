/* Projecting the actor offset onto the facing axis — 0x08023868-0x08023923
 *
 * Bits 4-5 of the record's +0x8A byte (a bitfield) are the quadrant; the
 * body's +0x68/+0x6C offsets are scaled by 280/256; two values are taken from
 * the gRom08CA30D8 sine table (at angle+256 and at angle) using the header's
 * +0x0E angle; for quadrant 2 the x offset is negated, and for kind 54
 * 0x100000 is added to y.  Result: body position + rotated offset (8.8
 * multiplications).
 *
 * TWO MEASUREMENTS: the table address must be taken into a SEPARATE LOCAL
 * first (the ROM loads it into r3 before the first index computation); the
 * >>8 shifts in the multiplications must be written INLINE -- taken into
 * separate locals, the shifts move ahead of the multiplication (94/97).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/project_actor_offset.c
 */

#include "gba_types.h"
#define ANGLE_MASK 0x3FF
#define SCALE      280
typedef struct Rec { u8 pad00[0x8A]; u8 low : 4; u8 quad : 2; u8 high : 2; } Rec;
typedef struct Body { s32 x; s32 y; u8 pad08[6]; s16 heading; u8 pad10[0x58]; s32 w68; s32 w6C; } Body;
typedef struct Actor { u8 pad00[9]; u8 kind; u8 pad0a[10]; Body *body; u8 pad18[4]; Rec *rec; } Actor;
extern const s16 gRom08CA30D8[];
void ProjectActorOffset(Actor *self, s32 *outX, s32 *outY)
{
    u32 q; Body *body; s32 sx; s32 sy; s32 heading; s32 sinA; s32 sinB; const s16 *tab;
    q = self->rec->quad;
    body = self->body;
    sx = (body->w68 * SCALE) >> 8;
    sy = (body->w6C * SCALE) >> 8;
    tab = gRom08CA30D8;
    heading = body->heading;
    sinA = tab[(heading + 256) & ANGLE_MASK] << 2;
    sinB = tab[heading & ANGLE_MASK] << 2;
    if (q == 2) {
        sx = -sx;
        if (self->kind == 54)
            sy += 0x100000;
    }
    *outX = body->x + ((sx >> 8) * (sinA >> 8) - (sy >> 8) * (sinB >> 8));
    *outY = body->y + ((sx >> 8) * (sinB >> 8) + (sy >> 8) * (sinA >> 8));
}
