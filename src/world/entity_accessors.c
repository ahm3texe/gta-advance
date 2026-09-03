/* Varlik erisimcileri — 0x08032090-0x080320BB
 *
 * Bes kucuk erisimci. Hepsi ayni yapiyi isaret eden bir isaretci aliyor;
 * alan adlari davranistan cikarildi, anlamlari henuz dogrulanmadi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entity_accessors.c
 */

#include "gba_types.h"

/* Alan ofsetleri ROM'dan okundu; isimler gecicidir. */
typedef struct {
    u8  flags : 7;  /* 0x00 — yedi bitlik alan; ROM lsls/lsrs cifti uretiyor */
    u8  unk00_7 : 1;
    u8  unk01;
    u8  kind;       /* 0x02 — ust nibble'i okunuyor */
    u8  unk03[3];
    u16 unk06;      /* 0x06 \                                        */
    u16 unk08;      /* 0x08  } ucu de 16.16 sabit noktaya cevriliyor */
    u16 unk0A;      /* 0x0A /                                        */
    u16 unk0C;      /* 0x0C */
    u16 unk0E;
    u32 unk10;      /* 0x10 */
    u16 unk14;
    u16 id;         /* 0x16 — bir tabanli; 0 "yok" demek */
} Entity;

/* 0x08032090 — uc alani 16.16 sabit noktaya cevirip hedefe yazar. */
void GetEntityFixedFields(const Entity *entity, u32 *out)
{
    out[0] = entity->unk06 << 16;
    out[1] = entity->unk08 << 16;
    out[2] = entity->unk0A << 16;
}

/* 0x080320A4 */
u16 GetEntityUnk0C(const Entity *entity)
{
    return entity->unk0C;
}

/* 0x080320A8 */
u32 GetEntityFlags(const Entity *entity)
{
    return entity->flags;
}

/* 0x080320B0 */
u32 GetEntityKind(const Entity *entity)
{
    return entity->kind >> 4;
}

/* 0x080320B8 */
u32 GetEntityUnk10(const Entity *entity)
{
    return entity->unk10;
}
