/* Aktor kaymasini yon ekseninde izdusurme — 0x08023868-0x08023923
 *
 * Kaydin +0x8A baytinin 4-5. bitleri (bitfield) cerek; govdenin +0x68/+0x6C
 * kaymalari 280/256 ile olceklenir; basligin +0x0E acisi ile
 * gRom08CA30D8 sinus tablosundan (aci+256 ve aci) iki deger alinir; cerek
 * 2 ise x kaymasi ters cevrilir ve tur 54 icin y'ye 0x100000 eklenir.
 * Cikis: govde konumu + dondurulmus kayma (8.8 carpimlar).
 *
 * IKI OLCUM: tablo adresi once AYRI YERELE alinmali (ROM r3'e ilk indis
 * hesabindan once yukluyor); carpimlardaki >>8 kaydirmalar SATIR ICI
 * yazilmali -- ayri yerellere alininca kaydirmalarin sirasi carpimdan once
 * kayiyor (94/97).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/project_actor_offset.c
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
