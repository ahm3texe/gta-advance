/* Aktor durum kontrolu + yon farki — 0x08019B08-0x08019B5B
 *
 * Ilk fonksiyon aktorun bayraklarina bakip "kullanilabilir" mi tespit
 * ediyor: +8 bit ise ozel dogrulama (return 1), +0 bit yoksa dur, +11 bit
 * varsa dur, alt yaslann + 48 = 2 ise dur, aksi 1.
 * Ikincisi FUN_08017D78 farkiyla 3 bit yon indeksi cikariyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/actor_check.c
 */

#include "gba_types.h"

#define FLAG_SPECIAL   0x0080
#define FLAG_READY     0x0001
#define FLAG_BLOCK     0x0800

typedef struct Sub {
    u8 pad00[48];
    u8 state;                   /* +0x30 */
} Sub;

typedef struct Actor {
    u8   pad00[12];
    u32  flags;                 /* +0x0C */
    u8   pad10[8];
    Sub *sub;                   /* +0x18 */
} Actor;

extern s32 FUN_08017d78(u32 arg);

/* 0x08019B08 */
u32 IsActorUsable(Actor *a)
{
    u32 flags;

    if (a == 0)
        return 0;

    flags = a->flags;
    if ((flags & FLAG_SPECIAL) != 0)
        return 1;
    if ((flags & FLAG_READY) == 0)
        return 0;
    if ((flags & FLAG_BLOCK) != 0)
        return 0;
    if (a->sub->state == 2)
        return 0;

    return 1;
}

/* 0x08019B3C */
s32 GetAngleField(u32 base, u32 arg)
{
    s32 diff;

    diff = FUN_08017d78(arg);
    return ((diff - (s32)base + 0x800000) >> 24) & 3;
}
