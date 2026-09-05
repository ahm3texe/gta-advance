/* Bayrak temizleyip bos ise bildirme — 0x08038000-0x0803801F
 *
 * +0x0C'deki 0x8000 bitini siliyor; +0x1C sifirsa ResetActor(0)
 * cagriliyor. Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/clear_flag_notify.c
 */

#include "gba_types.h"

#define BUSY_FLAG 0x8000

typedef struct Node {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
    u8  pad10[12];
    u32 sub;                    /* +0x1C */
} Node;

extern void ResetActor(u32 arg);

/* 0x08038000 */
void ClearFlagNotify(Node *node)
{
    node->flags &= ~BUSY_FLAG;
    if (node->sub == 0)
        ResetActor(0);
}
