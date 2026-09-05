/* Dugum havuzunu kurma — 0x08012AA4-0x08012B1F
 *
 * 128 dugumu cift bagli serbest listeye diziyor, aktif listeyi bosaltiyor,
 * onceki kare sayacini NODE_COUNT yapiyor ve OAM'i DMA ile sifirliyor.
 *
 * Ilk ve son dugum dongunun DISINDA ele aliniyor (prev=0 / next=0), aradaki
 * 126 dugum dongude: ROM sayaci 125'ten basliyor ve `bge` ile donuyor.
 *
 * Kural 43: sayac ve isaretci birlikte ilerliyorsa ikisi de `for`
 * artiriminda ve ROM sirasiyla (ROM: `subs r3,#1` sonra `adds r2,r1,#0`).
 *
 * DMA sifirlamasi yigindaki bir kelimeden sabit kaynakla yapiliyor
 * (`sub sp,#4` bunun icin); kontrol 0x85000100 = 0x100 kelime, 32-bit,
 * kaynak sabit.  REG_IME kaydedilip geri yukleniyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/init_node_pool.c
 */

#include "gba_io.h"
#include "sprite_pool.h"

#define OAM_CLEAR_CONTROL 0x85000100

/* 0x08012AA4 */
void InitNodePool(void)
{
    Node *node;
    s32   i;
    u16   ime;
    u32   zero;

    gNodePool.freeHead = &gNodePool.nodes[0];
    gNodePool.activeHead = 0;
    gNodePool.prevCount = NODE_COUNT;

    node = &gNodePool.nodes[0];
    node->next = node + 1;
    node->prev = 0;
    node++;

    for (i = NODE_COUNT - 3; i >= 0; i--, node++) {
        node->prev = node - 1;
        node->next = node + 1;
    }

    node->prev = node - 1;
    node->next = 0;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = &zero;
    REG_DMA3.dst = (void *)OAM_BASE;
    REG_DMA3.control = OAM_CLEAR_CONTROL;
    REG_DMA3.control;
    REG_IME = ime;
}
