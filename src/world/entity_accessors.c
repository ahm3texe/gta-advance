/* Entity accessors — 0x08032090-0x080320BB
 *
 * Five small accessors taking pointers to the same structure. Field names
 * were inferred from behavior; their meaning remains unverified.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entity_accessors.c
 */

#include "gba_types.h"

/* Offsets were read from the ROM; names are provisional. */
typedef struct {
    u8  flags : 7;  /* 0x00 — seven-bit field; the ROM emits an lsls/lsrs pair */
    u8  unk00_7 : 1;
    u8  unk01;
    u8  kind;       /* 0x02 — reads the high nibble */
    u8  unk03[3];
    u16 unk06;      /* 0x06 \                                        */
    u16 unk08;      /* 0x08 — all three are converted to 16.16 fixed-point */
    u16 unk0A;      /* 0x0A /                                        */
    u16 unk0C;      /* 0x0C */
    u16 unk0E;
    u32 unk10;      /* 0x10 */
    u16 unk14;
    u16 id;         /* 0x16 — one-based; 0 means none */
} Entity;

/* 0x08032090 — convert three fields to 16.16 fixed-point and write to the destination. */
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
