/* Oturum dugumu sifirlama — 0x0803C798-0x0803C7B9
 *
 * ESLESIYOR (byte-matching, 34 byte).
 *
 * Cozum: bayrak alani `u8` degil `s8`. ROM maskeyi 32 bit olarak kuruyor
 * (`movs r0,#5 / negs r0,r0` = -5), bayta daraltmiyor (`movs r0,#251`
 * degil). Sebep: agbcc/gcc `and`'in sabitini yalnizca AND'lanan degerin
 * ust bitlerinin sifir oldugunu BILDIGINDE daraltiyor. Alan `u8` iken
 * yukleme sifir-genisletme sayildigi icin nonzero_bits = 0xFF cikiyor ve
 * -5 -> 0xFB'ye iniyor. Alan isaretli oldugunda ust bitler bilinmiyor,
 * maske 32 bit kaliyor; deger geri `strb` ile yazildigi icin yukleme yine
 * `ldrb` olarak kaliyor.
 *
 * Ayni ciktiyi veren esdeger yazim: alani bitfield yapmak
 * (`u8 f0:2; u8 f2:1; u8 f3:5;` + `gSessionPtr->f2 = 0;`) — bitfield
 * ekleme/cikarma da maskeyi kelime kipinde kuruyor. Diger bitlerin anlami
 * bilinmedigi icin tek alanli isaretli bicim tercih edildi.
 *
 * Denenip TUTMAYANLAR (hepsi `movs r0,#251` uretti): `u8` alanda ~4, -5,
 * 0xFFFFFFFB maskeleri; `(u8)((s32)flags & ~4)` cast'i; u32 yerel uzerinden
 * okuma. `volatile u8` alan maskeyi daraltmakla kalmayip fazladan bir
 * `ldrb` de ekliyor. `s32` yerel maske dogru sabiti uretiyor ama araya
 * `add r0, r2, #0` kopyasi sokuyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/misc/session_node.c
 */

#include "gba_types.h"

typedef struct {
    u8  unk00[40];
    u32 unk28;          /* 0x28 */
    u32 unk2C;          /* 0x2C */
} Node;

typedef struct {
    u8    unk00[28];
    Node *node;         /* 0x1C */
} Context;

typedef struct {
    Context *context;   /* 0x00 */
    u8       unk04[4];
    s8       flags;     /* 0x08 */
} Session;

#define SESSION_FLAG_BIT  4
#define NODE_RESET_VALUE  24

extern Session *gSessionPtr;

/* 0x0803C798 */
void ResetSessionNode(void)
{
    Node *node;

    node = gSessionPtr->context->node;
    if (node != 0) {
        node->unk28 = NODE_RESET_VALUE;
        node->unk2C = 0;
    }

    gSessionPtr->flags &= ~SESSION_FLAG_BIT;
}
