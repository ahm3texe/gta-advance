/* Yuvayi yeniden kurma — 0x0803F69C-0x0803F6D7
 *
 * Ilk nesnenin +0x0C bayraklarinda bir biti kurup baskasini siliyor,
 * isleyici beklenenden farkliysa onu ayarlayip +0x32'yi sifirliyor,
 * degeri +0x2C'ye yaziyor ve +0x81'i sifirliyor.
 *
 * Uc kural birlikte:
 *   - kural 37: ROM hem `slot`u (r3) hem gelen degeri (r4) AYRI
 *     register'a kopyaliyor; ikisi de cagri boyunca degil ama coklu
 *     kullanim boyunca yasiyor
 *   - `__thumb` sonekli sembol: saklanan/karsilastirilan fonksiyon
 *     isaretcisinde bit 0 kurulu olmali (tools/agbcc_build.py)
 *   - kural 35: `pop {r0}; bx r0` -> donus tipi void
 *
 * Slot tanimi src/world/init_handler_pack.c ile BIREBIR AYNI olmali.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/rearm_slot.c
 */

#include "gba_types.h"

#define FLAG_SET   (0x80 << 16)
#define FLAG_CLEAR 0x01000000

typedef struct Pack12 {
    u32 a;
    u32 b;
    u32 c;
} Pack12;

typedef struct Slot {
    void  *first;               /* +0x00 */
    u8     pad04[4];
    void  *handler;             /* +0x08 */
    u8     pad0C[16];
    u32    value;               /* +0x1C */
    Pack12 pack;                /* +0x20 */
    u32    unk2C;               /* +0x2C */
    u8     pad30[2];
    u16    unk32;               /* +0x32 */
    u8     pad34[77];
    u8     ready;               /* +0x81 */
} Slot;

typedef struct Obj {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} Obj;

extern u8 FUN_0803d208__thumb[];

/* 0x0803F69C */
void RearmSlot(Slot *slot, u32 value)
{
    Obj *obj;

    obj = (Obj *)slot->first;
    obj->flags = (obj->flags | FLAG_SET) & ~FLAG_CLEAR;

    if (slot->handler != FUN_0803d208__thumb) {
        slot->handler = FUN_0803d208__thumb;
        slot->unk32 = 0;
    }

    slot->unk2C = value;
    slot->ready = 0;
}
