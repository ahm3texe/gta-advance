/* Aktoru yeniden kuruyor: gomulu alt yapiyi baglayip bayraklari temizliyor.
 * 0x08016768, 160 bayt.
 *
 * +0x3C alanina aktorun KENDI +0x40'indaki gomulu yapinin adresi yaziliyor,
 * sonra o yapi FUN_08014ffc ile kuruluyor. Kurulum bayragi iki parcadan
 * olusuyor: varligin +0x0C bayraklarindaki 0x140000 maskesi SIFIR DEGILSE
 * 0x80 biti, ve belli kosullarda +0x3E alanindan cikarilan uc bitlik bir
 * alan 9 kaydirilarak ekleniyor. Sonuca 3 eklenip geciliyor.
 *
 * Ghidra'nin sondaki "Could not recover jumptable" uyarisi yaniltici;
 * orada `pop {r0}; bx r0` interworking donusu var (kural 35 -> void).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/actor_reset.c
 */

#include "gba_types.h"

#define STATE_FLAG 0x140000
#define PART_BIT   0x80

typedef struct Detail {
    u8 pad0[0x3e];
    u16 bits;                 /* +0x3E */
} Detail;

typedef struct Entity {
    u8 pad0[8];
    u8 kind;                  /* +0x08 */
    u8 pad9[3];
    u32 flags;                /* +0x0C */
    Detail *detail;           /* +0x10 */
} Entity;

typedef struct Sub {
    u8 pad0[4];
} Sub;

typedef struct Actor {
    u8 *base;                 /* +0x00 */
    u8 pad4[4];
    u8 tag;                   /* +0x08 */
    u8 pad9[0x27];
    Entity *entity;           /* +0x30 */
    u8 pad34[8];
    Sub *sub;                 /* +0x3C */
    Sub  body;                /* +0x40 */
    u8 pad44[0x46];
    s8 lowA;                  /* +0x8A */
    u8 pad8b[0x1d];
    s8 lowB;                  /* +0xA8 */
} Actor;

extern void GetOwnerSlot(Entity *entity);
extern void FUN_08014ffc(Sub *sub, u32 flags, u8 *a, u8 *b);
extern void FUN_08015038(Sub *sub);
extern void UpdateActorFrame(Actor *self);

void ResetActor(Actor *self)
{
    Entity *ent;
    u32 mask;
    u32 flags;

    if (self == 0) return;

    ent = self->entity;
    self->sub = &self->body;
    /* ROM sonucu DEGISKENDE maddelestirip ayrica siniyor
     * (`movs r0,#0 / ... / movs r0,#1 / cmp r0,#0`); kisa devreli
     * `&&` yazimi dogrudan dallanma uretip alti bayt eksiltiyor. */
    if (ent != 0) {
        s32 ok = 0;
        if (ent->kind == 4) ok = 1;
        if (ok) GetOwnerSlot(ent);
    }

    mask = self->entity->flags & STATE_FLAG;
    flags = ((u32)((s32)(-mask | mask) >> 31)) & PART_BIT;
    if ((ent->flags & 0x200) != 0 || (self->entity->kind & 4) == 0) {
        flags |= ((ent->detail->bits >> 2) & 7) << 9;
    }
    FUN_08014ffc(self->sub, flags | 3, self->base, self->base + 0xc);
    FUN_08015038(self->sub);
    self->tag = 99;
    /* ROM maskeyi `movs #16 / negs` ile -16 olarak kuruyor, 0xF0 olarak
     * degil: kaynakta `~15` GENIS tipte bir yerelde tutuluyor.  Dogrudan
     * `&= ~15` yazilinca alan u8 oldugu icin derleyici sabiti 0xF0'a
     * indirgiyor ve `negs` komutu kayboluyor. */
    self->lowA &= ~15;
    self->lowB &= ~15;
    UpdateActorFrame(self);
}
